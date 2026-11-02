#pragma once
#include <Arduino.h>

// HID-Modifier-Bits
#define MOD_LCTRL  0x01
#define MOD_LSHIFT 0x02
#define MOD_LALT   0x04
#define MOD_LGUI   0x08
#define MOD_RALT   0x40  // AltGr

struct KeyStroke {
    uint8_t mod = 0;
    uint8_t key = 0;  // HID-Usage-Code
    bool dead = false;  // Tottaste (z. B. ^ auf DE) -> danach Leertaste senden
};

// Initialisiert USB (HID-Tastatur + Media-Tasten). Muss in setup() vor allem anderen USB-Kram laufen.
void keysBegin(const char *productName);
bool keysUsbReady();

// Sendet eine einzelne Kombination wie "CTRL+SHIFT+M". false = unbekannte Taste.
bool keysSendCombo(const String &combo, const String &layout, String &err);
// Sendet eine Folge "CTRL+K, CTRL+C"
bool keysSendSequence(const String &seq, const String &layout, String &err);
// Tippt Text (UTF-8, \n = Enter, \t = Tab)
void keysType(const String &text, const String &layout, uint16_t delayMs);
// Media-Taste per Name (VOL_UP, MUTE, ...). false = unbekannt.
bool keysMedia(const String &name);
