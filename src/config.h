#pragma once
#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

// Maximalwerte
#define LD_MAX_PAGES 12
#define LD_MAX_COLS  8
#define LD_MAX_ROWS  3

#define LONGDECK_VERSION "1.9.2"

// Aktionstypen einer Taste
enum ActionType : uint8_t {
    ACT_NONE = 0,
    ACT_HOTKEY,   // Tastenkombination(en), z. B. "CTRL+SHIFT+M" oder "CTRL+K, CTRL+C"
    ACT_MEDIA,    // Media-Taste, z. B. "VOL_UP"
    ACT_TEXT,     // Text tippen
    ACT_HTTP,     // Webhook/HTTP-Request – wird am PC per PowerShell (Win+R) ausgeführt
    ACT_PAGE,     // Zu Seite springen (value = Seitennummer, 1-basiert)
    ACT_MACRO,    // Mehrere Schritte, eine Zeile pro Schritt (key:/text:/media:/delay:/page:/open:)
    ACT_OPEN,     // Programm, Datei, Ordner oder Webseite öffnen (über Win+R)
    ACT_TOGGLE,   // Umschalter: führt "action" aus und wechselt danach Icon/Text/Farbe (states)
};

// Ein Zustand eines Umschalters (z. B. "Lautsprecher" / "Kopfhörer")
struct ToggleState {
    String label;
    String icon;
    uint32_t bg = 0x3B82F6;
};

struct DeckButton {
    String label;
    String icon;          // Name eines eingebauten Symbols (siehe icons.h), leer = keins
    uint32_t bg = 0x2A2F3A;
    uint32_t fg = 0xFFFFFF;
    ActionType type = ACT_NONE;
    String value;         // Kombination / Media-Name / Text / URL / Seite
    String method;        // HTTP: GET/POST/PUT
    String body;          // HTTP: Body
    String auth;          // HTTP: Bearer-Token (optional)
    String contentType;   // HTTP: Content-Type (Standard application/json)
    // Nur für Umschalter:
    ActionType action = ACT_HOTKEY;    // was beim Drücken ausgeführt wird
    std::vector<ToggleState> states;   // 2..6 Zustände
    uint8_t state = 0;                 // aktueller Zustand
};

// Anzeige einer Taste (beim Umschalter: Label/Icon/Farbe des aktuellen Zustands)
DeckButton deckButtonView(const DeckButton &b);
// Umschalter einen Zustand weiterschalten; true wenn geändert
bool configToggleAdvance(int page, int button);
// Verzögertes Speichern (z. B. nach Zustandswechsel), configSaveLoop() in loop() aufrufen
void configRequestSave();
void configSaveLoop();

struct DeckPage {
    String name;
    std::vector<DeckButton> buttons;  // rows*cols Einträge
};

struct DeckConfig {
    uint8_t cols = 5;
    uint8_t rows = 1;
    bool squareTiles = true;     // Kacheln nie höher als breit (Rand oben/unten statt langgezogen)
    String theme = "minimal";    // Skin: minimal, soft, bold, mono, glass, flat, frost*
    String swipe = "fast";       // Seitenwechsel: "fast" = sofort springen, "smooth" = Seite folgt dem Finger
    uint16_t rotation = 0;       // 0 = quer, 90 = hochkant, 180 = quer gedreht, 270 = hochkant gedreht
    String background = "auto"; // auto, black, aurora, midnight, graphite, ocean, sunset, forest, image
    uint8_t brightness = 200;    // 10..255
    uint32_t pwmFreq = 25000;    // Dimm-Frequenz der Beleuchtung in Hz, 0 = ohne Dimmen (nur an/aus)
    uint16_t dimAfter = 60;      // Sekunden bis Dimmen, 0 = nie
    uint8_t dimBrightness = 20;  // Helligkeit im gedimmten Zustand (0 = aus)
    String layout = "de";        // Tastaturlayout des PCs: "de" oder "us"
    uint16_t typeDelay = 8;      // ms zwischen Tastendrücken beim Tippen
    String deviceName = "LongDeck";
    std::vector<DeckPage> pages;
};

extern DeckConfig g_config;
extern SemaphoreHandle_t g_configMutex;
// Wird gesetzt, wenn die Konfiguration per USB geändert wurde -> UI neu aufbauen
extern volatile bool g_configChanged;

void configBegin();
bool configLoad();
bool configSave();
void configDefaults();
void configNormalize();  // sorgt für rows*cols Tasten pro Seite usw.

void configToJson(JsonDocument &doc);
bool configFromJson(JsonDocument &doc, String &err);

const char *actionTypeName(ActionType t);
ActionType actionTypeFromName(const String &s);
