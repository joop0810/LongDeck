#include "config.h"
#include <LittleFS.h>
#include "log.h"

DeckConfig g_config;
SemaphoreHandle_t g_configMutex = nullptr;
volatile bool g_configChanged = false;

static const char *CONFIG_PATH = "/config.json";

static const char *TYPE_NAMES[] = {"none", "hotkey", "media", "text", "http", "page", "macro", "open"};

const char *actionTypeName(ActionType t) {
    if (t > ACT_OPEN) return "none";
    return TYPE_NAMES[t];
}

ActionType actionTypeFromName(const String &s) {
    for (uint8_t i = 0; i <= ACT_OPEN; i++) {
        if (s.equalsIgnoreCase(TYPE_NAMES[i])) return (ActionType)i;
    }
    return ACT_NONE;
}

static String colorToHex(uint32_t c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06X", (unsigned)(c & 0xFFFFFF));
    return String(buf);
}

static uint32_t hexToColor(const char *s, uint32_t fallback) {
    if (!s || !*s) return fallback;
    if (*s == '#') s++;
    if (strlen(s) != 6) return fallback;
    char *end = nullptr;
    uint32_t v = strtoul(s, &end, 16);
    if (end == nullptr || *end != 0) return fallback;
    return v;
}

static DeckButton mk(const char *label, const char *icon, uint32_t bg, ActionType type, const char *value) {
    DeckButton b;
    b.label = label;
    b.icon = icon;
    b.bg = bg;
    b.type = type;
    b.value = value;
    return b;
}

void configDefaults() {
    g_config = DeckConfig();
    DeckPage p1;
    p1.name = "Start";
    p1.buttons = {
        mk("Leiser", "volume_mid", 0x1F4E79, ACT_MEDIA, "VOL_DOWN"),
        mk("Stumm", "mute", 0x7A1F2B, ACT_MEDIA, "MUTE"),
        mk("Lauter", "volume_max", 0x1F4E79, ACT_MEDIA, "VOL_UP"),
        mk("Zurück", "prev", 0x2E5E3A, ACT_MEDIA, "PREV"),
        mk("Play", "play", 0x2E7D32, ACT_MEDIA, "PLAY_PAUSE"),
        mk("Weiter", "next", 0x2E5E3A, ACT_MEDIA, "NEXT"),
        mk("Kopieren", "copy", 0x3A3F4B, ACT_HOTKEY, "CTRL+C"),
        mk("Einfügen", "paste", 0x3A3F4B, ACT_HOTKEY, "CTRL+V"),
        mk("Screenshot", "image", 0x5B3A7A, ACT_HOTKEY, "GUI+SHIFT+S"),
        mk("Task-\nManager", "list", 0x6B4E16, ACT_HOTKEY, "CTRL+SHIFT+ESC"),
        mk("Desktop", "home", 0x3A3F4B, ACT_HOTKEY, "GUI+D"),
        mk("Sperren", "power", 0x7A1F2B, ACT_HOTKEY, "GUI+L"),
    };
    DeckPage p2;
    p2.name = "Werkzeuge";
    p2.buttons = {
        mk("Explorer", "directory", 0x6B4E16, ACT_HOTKEY, "GUI+E"),
        mk("Editor", "edit", 0x3A3F4B, ACT_OPEN, "notepad"),
        mk("Einstell.", "settings", 0x3A3F4B, ACT_HOTKEY, "GUI+I"),
        mk("Rechner", "keyboard", 0x1F4E79, ACT_OPEN, "calc"),
        mk("Gruß", "edit", 0x2E5E3A, ACT_TEXT, "Viele Grüße\n"),
        mk("Mic aus", "mute", 0x7A1F2B, ACT_HOTKEY, "CTRL+SHIFT+M"),
        mk("Rückgängig", "loop", 0x3A3F4B, ACT_HOTKEY, "CTRL+Z"),
        mk("Wieder-\nholen", "refresh", 0x3A3F4B, ACT_HOTKEY, "CTRL+Y"),
        mk("Speichern", "save", 0x1F4E79, ACT_HOTKEY, "CTRL+S"),
        mk("Neuer Tab", "plus", 0x3A3F4B, ACT_HOTKEY, "CTRL+T"),
        mk("Alt+Tab", "shuffle", 0x3A3F4B, ACT_HOTKEY, "ALT+TAB"),
        mk("Start", "home", 0x2E7D32, ACT_PAGE, "1"),
    };
    g_config.pages.push_back(p1);
    g_config.pages.push_back(p2);
    configNormalize();
}

void configNormalize() {
    DeckConfig &c = g_config;
    c.cols = constrain(c.cols, 1, LD_MAX_COLS);
    c.rows = constrain(c.rows, 1, LD_MAX_ROWS);
    c.brightness = max<uint8_t>(c.brightness, 10);
    c.typeDelay = constrain(c.typeDelay, 2, 200);
    if (c.layout != "de" && c.layout != "us") c.layout = "de";
    if (c.deviceName.length() == 0) c.deviceName = "LongDeck";
    if (c.pages.empty()) {
        DeckPage p;
        p.name = "Seite 1";
        c.pages.push_back(p);
    }
    if (c.pages.size() > LD_MAX_PAGES) c.pages.resize(LD_MAX_PAGES);
    size_t n = (size_t)c.cols * c.rows;
    for (size_t i = 0; i < c.pages.size(); i++) {
        if (c.pages[i].name.length() == 0) c.pages[i].name = "Seite " + String(i + 1);
        c.pages[i].buttons.resize(n);
    }
}

void configToJson(JsonDocument &doc) {
    const DeckConfig &c = g_config;
    doc.clear();
    doc["version"] = 1;
    doc["deviceName"] = c.deviceName;
    doc["cols"] = c.cols;
    doc["rows"] = c.rows;
    doc["brightness"] = c.brightness;
    doc["dimAfter"] = c.dimAfter;
    doc["dimBrightness"] = c.dimBrightness;
    doc["layout"] = c.layout;
    doc["typeDelay"] = c.typeDelay;
    doc["squareTiles"] = c.squareTiles;
    JsonArray pages = doc["pages"].to<JsonArray>();
    for (const DeckPage &p : c.pages) {
        JsonObject jp = pages.add<JsonObject>();
        jp["name"] = p.name;
        JsonArray jb = jp["buttons"].to<JsonArray>();
        for (const DeckButton &b : p.buttons) {
            JsonObject o = jb.add<JsonObject>();
            o["label"] = b.label;
            o["icon"] = b.icon;
            o["bg"] = colorToHex(b.bg);
            o["fg"] = colorToHex(b.fg);
            o["type"] = actionTypeName(b.type);
            o["value"] = b.value;
            if (b.type == ACT_HTTP) {
                o["method"] = b.method;
                o["body"] = b.body;
                o["auth"] = b.auth;
                o["contentType"] = b.contentType;
            }
        }
    }
}

bool configFromJson(JsonDocument &doc, String &err) {
    if (!doc["pages"].is<JsonArray>()) {
        err = "pages fehlt";
        return false;
    }
    DeckConfig c;
    c.deviceName = doc["deviceName"] | "LongDeck";
    c.cols = doc["cols"] | 6;
    c.rows = doc["rows"] | 2;
    c.brightness = doc["brightness"] | 200;
    c.dimAfter = doc["dimAfter"] | 60;
    c.dimBrightness = doc["dimBrightness"] | 20;
    c.layout = doc["layout"] | "de";
    c.typeDelay = doc["typeDelay"] | 8;
    c.squareTiles = doc["squareTiles"] | true;
    for (JsonObject jp : doc["pages"].as<JsonArray>()) {
        DeckPage p;
        p.name = jp["name"] | "";
        for (JsonObject o : jp["buttons"].as<JsonArray>()) {
            DeckButton b;
            b.label = o["label"] | "";
            b.icon = o["icon"] | "";
            b.bg = hexToColor(o["bg"] | "", 0x2A2F3A);
            b.fg = hexToColor(o["fg"] | "", 0xFFFFFF);
            b.type = actionTypeFromName(o["type"] | "none");
            b.value = o["value"] | "";
            b.method = o["method"] | "POST";
            b.body = o["body"] | "";
            b.auth = o["auth"] | "";
            b.contentType = o["contentType"] | "application/json";
            p.buttons.push_back(b);
        }
        c.pages.push_back(p);
        if (c.pages.size() >= LD_MAX_PAGES) break;
    }
    g_config = c;
    configNormalize();
    return true;
}

bool configLoad() {
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f) return false;
    JsonDocument doc;
    DeserializationError e = deserializeJson(doc, f);
    f.close();
    if (e) {
        LOGF("[config] JSON-Fehler: %s\n", e.c_str());
        return false;
    }
    String err;
    if (!configFromJson(doc, err)) {
        LOGF("[config] ungueltig: %s\n", err.c_str());
        return false;
    }
    return true;
}

bool configSave() {
    JsonDocument doc;
    configToJson(doc);
    File f = LittleFS.open("/config.tmp", "w");
    if (!f) return false;
    size_t n = serializeJson(doc, f);
    f.close();
    if (n == 0) return false;
    LittleFS.remove(CONFIG_PATH);
    return LittleFS.rename("/config.tmp", CONFIG_PATH);
}

void configBegin() {
    g_configMutex = xSemaphoreCreateRecursiveMutex();
    if (!LittleFS.begin(true)) {
        LOGF("[config] LittleFS konnte nicht gemountet werden" "\n");
    }
    if (!configLoad()) {
        LOGF("[config] Standard-Konfiguration wird verwendet" "\n");
        configDefaults();
        configSave();
    }
}
