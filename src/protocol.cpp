#include "protocol.h"
#include "config.h"
#include "actions.h"
#include "icons.h"
#include "log.h"
#include "esp32-hal-tinyusb.h"

static String s_line;
static bool s_overflow = false;
static uint32_t s_lastSeen = 0;
static uint32_t s_restartAt = 0;
static bool s_toBootloader = false;

static const size_t MAX_LINE = 200 * 1024;  // Konfiguration kann einige 10 KB groß werden

static const char *MEDIA_NAMES[] = {
    "VOL_UP", "VOL_DOWN", "MUTE", "PLAY_PAUSE", "NEXT", "PREV", "STOP", "BRIGHT_UP",
    "BRIGHT_DOWN", "CALCULATOR", "BROWSER", "MAIL", "MY_COMPUTER", "WWW_HOME", "WWW_BACK",
    "WWW_FORWARD", "WWW_REFRESH", "WWW_SEARCH",
};

static void reply(JsonDocument &doc) {
    String out;
    serializeJson(doc, out);
    out += '\n';
    const uint8_t *p = (const uint8_t *)out.c_str();
    size_t left = out.length();
    uint32_t start = millis();
    while (left > 0 && millis() - start < 5000) {
        size_t n = USBSerial.write(p, left);
        p += n;
        left -= n;
        if (n == 0) delay(2);
    }
}

static void replyError(int id, const String &msg) {
    JsonDocument d;
    d["id"] = id;
    d["ok"] = false;
    d["error"] = msg;
    reply(d);
}

static void handle(const String &line) {
    JsonDocument req;
    DeserializationError e = deserializeJson(req, line);
    if (e) return replyError(0, String("JSON: ") + e.c_str());
    int id = req["id"] | 0;
    String cmd = req["cmd"] | "";
    s_lastSeen = millis();

    JsonDocument res;
    res["id"] = id;
    res["ok"] = true;

    if (cmd == "hello" || cmd == "ping") {
        res["device"] = "LongDeck";
        res["version"] = LONGDECK_VERSION;
        if (cmd == "hello") {
            JsonArray icons = res["icons"].to<JsonArray>();
            for (const DeckIcon &i : DECK_ICONS) icons.add(i.name);
            JsonArray media = res["media"].to<JsonArray>();
            for (const char *m : MEDIA_NAMES) media.add(m);
            res["maxPages"] = LD_MAX_PAGES;
            res["maxCols"] = LD_MAX_COLS;
            res["maxRows"] = LD_MAX_ROWS;
        }
    } else if (cmd == "get") {
        JsonDocument cfg;
        xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
        configToJson(cfg);
        xSemaphoreGiveRecursive(g_configMutex);
        res["config"] = cfg;
    } else if (cmd == "set") {
        if (!req["config"].is<JsonObject>()) return replyError(id, "config fehlt");
        JsonDocument cfg;
        cfg.set(req["config"]);
        xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
        String err;
        bool ok = configFromJson(cfg, err);
        if (ok && !configSave()) { ok = false; err = "Speichern fehlgeschlagen"; }
        xSemaphoreGiveRecursive(g_configMutex);
        if (!ok) return replyError(id, err);
        g_configChanged = true;
    } else if (cmd == "press") {
        actionsTrigger(req["page"] | 0, req["btn"] | 0);
    } else if (cmd == "reset") {
        xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
        configDefaults();
        configSave();
        xSemaphoreGiveRecursive(g_configMutex);
        g_configChanged = true;
    } else if (cmd == "reboot") {
        s_restartAt = millis() + 300;
    } else if (cmd == "bootloader") {
        s_toBootloader = true;
        s_restartAt = millis() + 300;
    } else {
        return replyError(id, "Unbekannter Befehl: " + cmd);
    }
    reply(res);
}

void protocolBegin() {
    s_line.reserve(1024);
}

void protocolLoop() {
    // Begrenzt pro Durchlauf, damit die Oberfläche flüssig bleibt
    int budget = 8192;
    while (USBSerial.available() > 0 && budget-- > 0) {
        char c = (char)USBSerial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            if (s_overflow) replyError(0, "Zeile zu lang");
            else if (s_line.length()) handle(s_line);
            s_line = "";
            s_overflow = false;
            continue;
        }
        if (s_line.length() < MAX_LINE) s_line += c;
        else s_overflow = true;
    }
    if (s_restartAt && millis() > s_restartAt) {
        USBSerial.flush();
        delay(100);
        if (s_toBootloader) usb_persist_restart(RESTART_BOOTLOADER);
        ESP.restart();
    }
}

bool protocolEditorConnected() {
    return s_lastSeen && millis() - s_lastSeen < 6000;
}
