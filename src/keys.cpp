#include "keys.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDConsumerControl.h"

#include "log.h"

USBCDC USBSerial;  // serielle Konsole + ermöglicht automatisches Flashen (esptool-Reset über USB)
static USBHIDKeyboard s_keyboard;
static USBHIDConsumerControl s_consumer;
static USBHID s_hid;

void keysBegin(const char *productName) {
    USBSerial.begin(115200);
    s_keyboard.begin();
    s_consumer.begin();
    USB.productName(productName);
    USB.manufacturerName("LongDeck");
    USB.begin();
}

bool keysUsbReady() { return s_hid.ready(); }

// ---------------------------------------------------------------------------
// Zeichen -> Taste je Layout
// ---------------------------------------------------------------------------
static bool letterStroke(char c, bool de, KeyStroke &ks) {
    char lc = tolower(c);
    if (lc < 'a' || lc > 'z') return false;
    uint8_t code = 0x04 + (lc - 'a');
    if (de) {
        if (lc == 'y') code = 0x1D;      // DE: Y liegt auf US-Z
        else if (lc == 'z') code = 0x1C; // DE: Z liegt auf US-Y
    }
    ks.key = code;
    ks.mod = isupper(c) ? MOD_LSHIFT : 0;
    return true;
}

static bool digitStroke(char c, KeyStroke &ks) {
    if (c < '0' || c > '9') return false;
    ks.key = (c == '0') ? 0x27 : (0x1E + (c - '1'));
    ks.mod = 0;
    return true;
}

struct CharMap { uint32_t cp; uint8_t mod; uint8_t key; bool dead; };

static const CharMap US_MAP[] = {
    {' ', 0, 0x2C, false}, {'!', MOD_LSHIFT, 0x1E, false}, {'"', MOD_LSHIFT, 0x34, false},
    {'#', MOD_LSHIFT, 0x20, false}, {'$', MOD_LSHIFT, 0x21, false}, {'%', MOD_LSHIFT, 0x22, false},
    {'&', MOD_LSHIFT, 0x24, false}, {'\'', 0, 0x34, false}, {'(', MOD_LSHIFT, 0x26, false},
    {')', MOD_LSHIFT, 0x27, false}, {'*', MOD_LSHIFT, 0x25, false}, {'+', MOD_LSHIFT, 0x2E, false},
    {',', 0, 0x36, false}, {'-', 0, 0x2D, false}, {'.', 0, 0x37, false}, {'/', 0, 0x38, false},
    {':', MOD_LSHIFT, 0x33, false}, {';', 0, 0x33, false}, {'<', MOD_LSHIFT, 0x36, false},
    {'=', 0, 0x2E, false}, {'>', MOD_LSHIFT, 0x37, false}, {'?', MOD_LSHIFT, 0x38, false},
    {'@', MOD_LSHIFT, 0x1F, false}, {'[', 0, 0x2F, false}, {'\\', 0, 0x31, false},
    {']', 0, 0x30, false}, {'^', MOD_LSHIFT, 0x23, false}, {'_', MOD_LSHIFT, 0x2D, false},
    {'`', 0, 0x35, false}, {'{', MOD_LSHIFT, 0x2F, false}, {'|', MOD_LSHIFT, 0x31, false},
    {'}', MOD_LSHIFT, 0x30, false}, {'~', MOD_LSHIFT, 0x35, false},
};

static const CharMap DE_MAP[] = {
    {' ', 0, 0x2C, false}, {'!', MOD_LSHIFT, 0x1E, false}, {'"', MOD_LSHIFT, 0x1F, false},
    {'#', 0, 0x32, false}, {'$', MOD_LSHIFT, 0x21, false}, {'%', MOD_LSHIFT, 0x22, false},
    {'&', MOD_LSHIFT, 0x23, false}, {'\'', MOD_LSHIFT, 0x32, false}, {'(', MOD_LSHIFT, 0x25, false},
    {')', MOD_LSHIFT, 0x26, false}, {'*', MOD_LSHIFT, 0x30, false}, {'+', 0, 0x30, false},
    {',', 0, 0x36, false}, {'-', 0, 0x38, false}, {'.', 0, 0x37, false}, {'/', MOD_LSHIFT, 0x24, false},
    {':', MOD_LSHIFT, 0x37, false}, {';', MOD_LSHIFT, 0x36, false}, {'<', 0, 0x64, false},
    {'=', MOD_LSHIFT, 0x27, false}, {'>', MOD_LSHIFT, 0x64, false}, {'?', MOD_LSHIFT, 0x2D, false},
    {'@', MOD_RALT, 0x14, false}, {'[', MOD_RALT, 0x25, false}, {'\\', MOD_RALT, 0x2D, false},
    {']', MOD_RALT, 0x26, false}, {'^', 0, 0x35, true}, {'_', MOD_LSHIFT, 0x38, false},
    {'`', MOD_LSHIFT, 0x2E, true}, {'{', MOD_RALT, 0x24, false}, {'|', MOD_RALT, 0x64, false},
    {'}', MOD_RALT, 0x27, false}, {'~', MOD_RALT, 0x30, false},
    {0xE4, 0, 0x34, false}, {0xF6, 0, 0x33, false}, {0xFC, 0, 0x2F, false},          // ä ö ü
    {0xC4, MOD_LSHIFT, 0x34, false}, {0xD6, MOD_LSHIFT, 0x33, false}, {0xDC, MOD_LSHIFT, 0x2F, false},  // Ä Ö Ü
    {0xDF, 0, 0x2D, false}, {0xA7, MOD_LSHIFT, 0x20, false}, {0xB0, MOD_LSHIFT, 0x35, false},  // ß § °
    {0x20AC, MOD_RALT, 0x08, false}, {0xB2, MOD_RALT, 0x1F, false}, {0xB3, MOD_RALT, 0x20, false},  // € ² ³
    {0xB5, MOD_RALT, 0x10, false}, {0xB4, 0, 0x2E, true},  // µ ´
};

static bool charStroke(uint32_t cp, bool de, KeyStroke &ks) {
    ks = KeyStroke();
    if (cp < 128 && letterStroke((char)cp, de, ks)) return true;
    if (cp < 128 && digitStroke((char)cp, ks)) return true;
    if (cp == '\n') { ks.key = 0x28; return true; }
    if (cp == '\t') { ks.key = 0x2B; return true; }
    const CharMap *map = de ? DE_MAP : US_MAP;
    size_t n = de ? sizeof(DE_MAP) / sizeof(CharMap) : sizeof(US_MAP) / sizeof(CharMap);
    for (size_t i = 0; i < n; i++) {
        if (map[i].cp == cp) {
            ks.mod = map[i].mod;
            ks.key = map[i].key;
            ks.dead = map[i].dead;
            return true;
        }
    }
    return false;
}

// Dekodiert ein UTF-8-Zeichen ab Position i, i wird weitergesetzt
static uint32_t nextCodepoint(const String &s, size_t &i) {
    uint8_t c = s[i++];
    if (c < 0x80) return c;
    int extra = 0;
    uint32_t cp = 0;
    if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else return 0xFFFD;
    while (extra-- > 0 && i < s.length()) cp = (cp << 6) | (s[i++] & 0x3F);
    return cp;
}

// ---------------------------------------------------------------------------
// Benannte Tasten
// ---------------------------------------------------------------------------
struct NamedKey { const char *name; uint8_t key; };
static const NamedKey NAMED_KEYS[] = {
    {"ENTER", 0x28}, {"RETURN", 0x28}, {"ESC", 0x29}, {"ESCAPE", 0x29}, {"BACKSPACE", 0x2A},
    {"BKSP", 0x2A}, {"TAB", 0x2B}, {"SPACE", 0x2C}, {"LEERTASTE", 0x2C}, {"CAPSLOCK", 0x39},
    {"PRINTSCREEN", 0x46}, {"PRINT", 0x46}, {"DRUCK", 0x46}, {"SCROLLLOCK", 0x47}, {"PAUSE", 0x48},
    {"INSERT", 0x49}, {"INS", 0x49}, {"EINFG", 0x49}, {"HOME", 0x4A}, {"POS1", 0x4A},
    {"PGUP", 0x4B}, {"PAGEUP", 0x4B}, {"BILDAUF", 0x4B}, {"DELETE", 0x4C}, {"DEL", 0x4C},
    {"ENTF", 0x4C}, {"END", 0x4D}, {"ENDE", 0x4D}, {"PGDN", 0x4E}, {"PAGEDOWN", 0x4E},
    {"BILDAB", 0x4E}, {"RIGHT", 0x4F}, {"LEFT", 0x50}, {"DOWN", 0x51}, {"UP", 0x52},
    {"NUMLOCK", 0x53}, {"NUMDIV", 0x54}, {"NUMMULT", 0x55}, {"NUMMINUS", 0x56}, {"NUMPLUS", 0x57},
    {"NUMENTER", 0x58}, {"NUM1", 0x59}, {"NUM2", 0x5A}, {"NUM3", 0x5B}, {"NUM4", 0x5C},
    {"NUM5", 0x5D}, {"NUM6", 0x5E}, {"NUM7", 0x5F}, {"NUM8", 0x60}, {"NUM9", 0x61},
    {"NUM0", 0x62}, {"NUMDOT", 0x63}, {"MENU", 0x65}, {"APP", 0x65},
};

static bool parseModifier(const String &t, uint8_t &mod) {
    if (t == "CTRL" || t == "CONTROL" || t == "STRG") { mod |= MOD_LCTRL; return true; }
    if (t == "SHIFT") { mod |= MOD_LSHIFT; return true; }
    if (t == "ALT") { mod |= MOD_LALT; return true; }
    if (t == "ALTGR") { mod |= MOD_RALT; return true; }
    if (t == "GUI" || t == "WIN" || t == "CMD" || t == "META" || t == "SUPER") { mod |= MOD_LGUI; return true; }
    return false;
}

static bool parseKeyToken(const String &raw, bool de, KeyStroke &ks) {
    String t = raw;
    t.toUpperCase();
    for (const NamedKey &nk : NAMED_KEYS) {
        if (t == nk.name) { ks.key = nk.key; ks.mod = 0; return true; }
    }
    // F1..F24
    if (t.length() >= 2 && t[0] == 'F' && isdigit(t[1])) {
        int n = t.substring(1).toInt();
        if (n >= 1 && n <= 12) { ks.key = 0x3A + (n - 1); ks.mod = 0; return true; }
        if (n >= 13 && n <= 24) { ks.key = 0x68 + (n - 13); ks.mod = 0; return true; }
    }
    if (t == "PLUS") return charStroke('+', de, ks);
    if (t == "MINUS") return charStroke('-', de, ks);
    // Einzelnes Zeichen (auch UTF-8 wie Ä)
    size_t i = 0;
    uint32_t cp = nextCodepoint(raw, i);
    if (i == raw.length()) {
        // Buchstaben in Kombinationen immer klein interpretieren (Shift explizit angeben)
        if (cp < 128 && isalpha(cp)) cp = tolower(cp);
        return charStroke(cp, de, ks);
    }
    return false;
}

static void sendReport(uint8_t mod, uint8_t key) {
    KeyReport r = {};
    r.modifiers = mod;
    r.keys[0] = key;
    s_keyboard.sendReport(&r);
}

static void tapStroke(const KeyStroke &ks, uint16_t holdMs) {
    if (ks.mod) {
        sendReport(ks.mod, 0);
        delay(4);
    }
    sendReport(ks.mod, ks.key);
    delay(holdMs);
    if (ks.mod) {
        sendReport(ks.mod, 0);
        delay(4);
    }
    sendReport(0, 0);
}

bool keysSendCombo(const String &comboIn, const String &layout, String &err) {
    bool de = layout == "de";
    String combo = comboIn;
    combo.trim();
    if (combo.length() == 0) { err = "leere Kombination"; return false; }

    String keyPart;
    String modPart;
    if (combo.endsWith("++") || combo == "+") {
        keyPart = "+";
        modPart = combo.length() > 2 ? combo.substring(0, combo.length() - 2) : "";
    } else {
        int p = combo.lastIndexOf('+');
        if (p < 0) { keyPart = combo; }
        else { keyPart = combo.substring(p + 1); modPart = combo.substring(0, p); }
    }
    keyPart.trim();

    uint8_t mod = 0;
    int start = 0;
    while (modPart.length() > 0 && start <= (int)modPart.length()) {
        int p = modPart.indexOf('+', start);
        String t = p < 0 ? modPart.substring(start) : modPart.substring(start, p);
        t.trim();
        t.toUpperCase();
        if (t.length() && !parseModifier(t, mod)) { err = "unbekannter Modifier: " + t; return false; }
        if (p < 0) break;
        start = p + 1;
    }

    KeyStroke ks;
    String upperKey = keyPart;
    upperKey.toUpperCase();
    uint8_t onlyMod = 0;
    if (parseModifier(upperKey, onlyMod)) {
        // Nur Modifier (z. B. "WIN" für das Startmenü)
        ks.key = 0;
        ks.mod = mod | onlyMod;
        sendReport(ks.mod, 0);
        delay(30);
        sendReport(0, 0);
        return true;
    }
    if (!parseKeyToken(keyPart, de, ks)) { err = "unbekannte Taste: " + keyPart; return false; }
    ks.mod |= mod;
    tapStroke(ks, 30);
    return true;
}

bool keysSendSequence(const String &seq, const String &layout, String &err) {
    int start = 0;
    bool any = false;
    while (start <= (int)seq.length()) {
        int p = seq.indexOf(',', start);
        String part = p < 0 ? seq.substring(start) : seq.substring(start, p);
        part.trim();
        // ",," -> Komma als Taste
        if (part.length() == 0 && p >= 0 && p + 1 < (int)seq.length() && seq[p + 1] == ',') {
            part = ",";
            p++;
        }
        if (part.length()) {
            if (!keysSendCombo(part, layout, err)) return false;
            any = true;
            delay(40);
        }
        if (p < 0) break;
        start = p + 1;
    }
    if (!any) err = "keine Taste angegeben";
    return any;
}

void keysType(const String &text, const String &layout, uint16_t delayMs) {
    bool de = layout == "de";
    size_t i = 0;
    while (i < text.length()) {
        uint32_t cp = nextCodepoint(text, i);
        if (cp == '\r') continue;
        KeyStroke ks;
        if (!charStroke(cp, de, ks)) continue;  // nicht darstellbares Zeichen überspringen
        tapStroke(ks, delayMs);
        delay(delayMs);
        if (ks.dead) {
            KeyStroke sp;
            sp.key = 0x2C;
            tapStroke(sp, delayMs);
            delay(delayMs);
        }
    }
}

struct MediaKey { const char *name; uint16_t code; };
static const MediaKey MEDIA_KEYS[] = {
    {"VOL_UP", 0x00E9}, {"VOL_DOWN", 0x00EA}, {"MUTE", 0x00E2}, {"PLAY_PAUSE", 0x00CD},
    {"NEXT", 0x00B5}, {"PREV", 0x00B6}, {"STOP", 0x00B7}, {"BRIGHT_UP", 0x006F},
    {"BRIGHT_DOWN", 0x0070}, {"CALCULATOR", 0x0192}, {"BROWSER", 0x0196}, {"MAIL", 0x018A},
    {"MY_COMPUTER", 0x0194}, {"WWW_HOME", 0x0223}, {"WWW_BACK", 0x0224}, {"WWW_FORWARD", 0x0225},
    {"WWW_REFRESH", 0x0227}, {"WWW_SEARCH", 0x0221},
};

bool keysMedia(const String &nameIn) {
    String name = nameIn;
    name.trim();
    name.toUpperCase();
    for (const MediaKey &m : MEDIA_KEYS) {
        if (name == m.name) {
            s_consumer.press(m.code);
            delay(30);
            s_consumer.release();
            return true;
        }
    }
    return false;
}
