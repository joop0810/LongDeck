#include "config.h"
#include <LittleFS.h>
#include "log.h"
#include "icons.h"

DeckConfig g_config;
SemaphoreHandle_t g_configMutex = nullptr;
volatile bool g_configChanged = false;

static const char *CONFIG_PATH = "/config.json";

static const char *TYPE_NAMES[] = {"none", "hotkey", "media", "text", "http", "page", "macro", "open", "toggle"};

const char *actionTypeName(ActionType t) {
    if (t > ACT_TOGGLE) return "none";
    return TYPE_NAMES[t];
}

ActionType actionTypeFromName(const String &s) {
    for (uint8_t i = 0; i <= ACT_TOGGLE; i++) {
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
    p1.name = "Medien";
    p1.buttons = {
        mk("Leiser", "volume-2", 0x3B82F6, ACT_MEDIA, "VOL_DOWN"),
        mk("Stumm", "volume-off", 0xEF4444, ACT_MEDIA, "MUTE"),
        mk("Lauter", "volume", 0x3B82F6, ACT_MEDIA, "VOL_UP"),
        mk("Play", "player-play", 0x22C55E, ACT_MEDIA, "PLAY_PAUSE"),
        mk("Weiter", "player-track-next", 0x22C55E, ACT_MEDIA, "NEXT"),
    };
    DeckPage p2;
    p2.name = "Arbeit";
    p2.buttons = {
        mk("Kopieren", "copy", 0xA1A1AA, ACT_HOTKEY, "CTRL+C"),
        mk("Einfügen", "clipboard", 0xA1A1AA, ACT_HOTKEY, "CTRL+V"),
        mk("Screenshot", "screenshot", 0xA855F7, ACT_HOTKEY, "GUI+SHIFT+S"),
        mk("Mic aus", "microphone-off", 0xEF4444, ACT_HOTKEY, "CTRL+SHIFT+M"),
        mk("Sperren", "lock", 0xF59E0B, ACT_HOTKEY, "GUI+L"),
    };
    DeckPage p3;
    p3.name = "Werkzeuge";
    p3.buttons = {
        mk("Explorer", "folder", 0xF59E0B, ACT_HOTKEY, "GUI+E"),
        mk("Rechner", "calculator", 0x3B82F6, ACT_OPEN, "calc"),
        mk("Editor", "pencil", 0xA1A1AA, ACT_OPEN, "notepad"),
        mk("Task-Manager", "list-check", 0xA855F7, ACT_HOTKEY, "CTRL+SHIFT+ESC"),
        mk("Desktop", "home", 0x22C55E, ACT_HOTKEY, "GUI+D"),
    };
    g_config.pages.push_back(p1);
    g_config.pages.push_back(p2);
    g_config.pages.push_back(p3);
    configNormalize();
}

void configNormalize() {
    DeckConfig &c = g_config;
    c.cols = constrain(c.cols, 1, LD_MAX_COLS);
    c.rows = constrain(c.rows, 1, LD_MAX_ROWS);
    c.brightness = max<uint8_t>(c.brightness, 10);
    c.typeDelay = constrain(c.typeDelay, 2, 200);
    if (c.layout != "de" && c.layout != "us") c.layout = "de";
    if (c.theme != "minimal" && c.theme != "soft" && c.theme != "bold" && c.theme != "mono" && c.theme != "glass" && c.theme != "flat" && !c.theme.startsWith("frost") && c.theme != "dash") c.theme = "minimal";
    if (c.background.length() == 0) c.background = "auto";
    c.rotation = (c.rotation / 90 % 4) * 90;
    if (c.swipe != "smooth") c.swipe = "fast";
    if (c.pwmFreq != 0) c.pwmFreq = constrain(c.pwmFreq, 1000, 60000);
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
        for (DeckButton &b : c.pages[i].buttons) {
            if (b.icon.length()) b.icon = iconCanonicalName(b.icon.c_str());  // alte Symbolnamen umstellen
        }
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
    doc["theme"] = c.theme;
    doc["background"] = c.background;
    doc["rotation"] = c.rotation;
    doc["swipe"] = c.swipe;
    doc["pwmFreq"] = c.pwmFreq;
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
            if (b.type == ACT_TOGGLE) {
                o["action"] = actionTypeName(b.action);
                o["state"] = b.state;
                JsonArray st = o["states"].to<JsonArray>();
                for (const ToggleState &t : b.states) {
                    JsonObject so = st.add<JsonObject>();
                    so["label"] = t.label;
                    so["icon"] = t.icon;
                    so["bg"] = colorToHex(t.bg);
                }
            }
            if (b.type == ACT_HTTP || (b.type == ACT_TOGGLE && b.action == ACT_HTTP)) {
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
    c.cols = doc["cols"] | 5;
    c.rows = doc["rows"] | 1;
    c.brightness = doc["brightness"] | 200;
    c.dimAfter = doc["dimAfter"] | 60;
    c.dimBrightness = doc["dimBrightness"] | 20;
    c.layout = doc["layout"] | "de";
    c.typeDelay = doc["typeDelay"] | 8;
    c.squareTiles = doc["squareTiles"] | true;
    c.theme = doc["theme"] | "minimal";
    c.background = doc["background"] | "auto";
    c.rotation = doc["rotation"] | 0;
    c.swipe = doc["swipe"] | "fast";
    c.pwmFreq = doc["pwmFreq"] | 25000;
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
            if (b.type == ACT_TOGGLE) {
                b.action = actionTypeFromName(o["action"] | "hotkey");
                if (b.action == ACT_TOGGLE || b.action == ACT_PAGE) b.action = ACT_HOTKEY;
                for (JsonObject so : o["states"].as<JsonArray>()) {
                    ToggleState t;
                    t.label = so["label"] | "";
                    t.icon = so["icon"] | "";
                    t.bg = hexToColor(so["bg"] | "", 0x3B82F6);
                    b.states.push_back(t);
                    if (b.states.size() >= 6) break;
                }
                b.state = o["state"] | 0;
                if (b.states.empty()) b.state = 0;
                else if (b.state >= b.states.size()) b.state = 0;
            }
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

DeckButton deckButtonView(const DeckButton &b) {
    DeckButton v = b;
    if (b.type == ACT_TOGGLE && !b.states.empty()) {
        const ToggleState &t = b.states[b.state < b.states.size() ? b.state : 0];
        v.label = t.label;
        v.icon = t.icon;
        v.bg = t.bg;
    }
    return v;
}

bool configToggleAdvance(int page, int button) {
    if (page < 0 || page >= (int)g_config.pages.size()) return false;
    if (button < 0 || button >= (int)g_config.pages[page].buttons.size()) return false;
    DeckButton &b = g_config.pages[page].buttons[button];
    if (b.type != ACT_TOGGLE || b.states.size() < 2) return false;
    b.state = (b.state + 1) % b.states.size();
    return true;
}

static uint32_t s_saveAt = 0;
void configRequestSave() { s_saveAt = millis() + 3000; }
void configSaveLoop() {
    if (s_saveAt && millis() > s_saveAt) {
        s_saveAt = 0;
        xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
        configSave();
        xSemaphoreGiveRecursive(g_configMutex);
    }
}
