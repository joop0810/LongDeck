#include "protocol.h"
#include "config.h"
#include "actions.h"
#include "icons.h"
#include "log.h"
#include "esp32-hal-tinyusb.h"
#include <LittleFS.h>
#include "mbedtls/base64.h"
#include <Update.h>

static String s_line;
static bool s_overflow = false;
static uint32_t s_lastSeen = 0;
static uint32_t s_restartAt = 0;
static bool s_toBootloader = false;
static File s_bgFile;
static size_t s_bgWritten = 0;
static const size_t BG_SIZE = 640 * 180 * 2;
static size_t s_otaSize = 0, s_otaWritten = 0;
static bool s_otaActive = false;

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
            res["hasBgImage"] = LittleFS.exists("/bg.bin");
            res["ota"] = true;  // Firmware-Update über diese Verbindung möglich
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
    } else if (cmd == "otabegin") {
        // Firmware-Update über USB: nur die App-Partition wird beschrieben, Belegung bleibt erhalten
        s_otaSize = req["size"] | 0;
        if (s_otaSize < 100000) return replyError(id, "Groesse fehlt");
        if (s_otaActive) Update.abort();
        if (!Update.begin(s_otaSize, U_FLASH)) return replyError(id, String("Update.begin: ") + Update.errorString());
        s_otaActive = true;
        s_otaWritten = 0;
        LOGF("[ota] start, %u Bytes\n", (unsigned)s_otaSize);
    } else if (cmd == "otachunk") {
        if (!s_otaActive) return replyError(id, "otabegin fehlt");
        const char *data = req["data"] | "";
        size_t outLen = 0;
        static uint8_t obuf[12288];
        if (mbedtls_base64_decode(obuf, sizeof(obuf), &outLen, (const unsigned char *)data, strlen(data)) != 0)
            return replyError(id, "Base64 ungueltig");
        if (Update.write(obuf, outLen) != outLen) {
            String e = Update.errorString();
            Update.abort();
            s_otaActive = false;
            return replyError(id, "Schreibfehler: " + e);
        }
        s_otaWritten += outLen;
        res["written"] = (uint32_t)s_otaWritten;
    } else if (cmd == "otaend") {
        if (!s_otaActive) return replyError(id, "otabegin fehlt");
        s_otaActive = false;
        if (s_otaWritten != s_otaSize || !Update.end(true)) {
            String e = Update.errorString();
            Update.abort();
            return replyError(id, "Update fehlgeschlagen: " + e);
        }
        LOGF("[ota] fertig, Neustart\n");
        s_restartAt = millis() + 600;
    } else if (cmd == "bgbegin") {
        // Hintergrundbild hochladen: 640x180 RGB565 (LE), in Stücken als Base64
        if (s_bgFile) s_bgFile.close();
        s_bgFile = LittleFS.open("/bg.tmp", "w");
        s_bgWritten = 0;
        if (!s_bgFile) return replyError(id, "Datei konnte nicht angelegt werden");
    } else if (cmd == "bgchunk") {
        if (!s_bgFile) return replyError(id, "bgbegin fehlt");
        const char *data = req["data"] | "";
        size_t inLen = strlen(data), outLen = 0;
        static uint8_t buf[6144];
        if (mbedtls_base64_decode(buf, sizeof(buf), &outLen, (const unsigned char *)data, inLen) != 0)
            return replyError(id, "Base64 ungueltig");
        if (s_bgWritten + outLen > BG_SIZE) return replyError(id, "Bild zu gross");
        if (s_bgFile.write(buf, outLen) != outLen) return replyError(id, "Schreibfehler (Speicher voll?)");
        s_bgWritten += outLen;
        res["written"] = (uint32_t)s_bgWritten;
    } else if (cmd == "bgend") {
        if (!s_bgFile) return replyError(id, "bgbegin fehlt");
        s_bgFile.close();
        if (s_bgWritten != BG_SIZE) {
            LittleFS.remove("/bg.tmp");
            return replyError(id, "Bild unvollstaendig");
        }
        LittleFS.remove("/bg.bin");
        LittleFS.rename("/bg.tmp", "/bg.bin");
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
