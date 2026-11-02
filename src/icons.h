#pragma once
#include "lvgl.h"
#include <string.h>

// Eingebaute Symbole (FontAwesome-Auswahl aus LVGL). Name -> Zeichen.
struct DeckIcon { const char *name; const char *symbol; };

static const DeckIcon DECK_ICONS[] = {
    {"audio", LV_SYMBOL_AUDIO}, {"video", LV_SYMBOL_VIDEO}, {"list", LV_SYMBOL_LIST},
    {"ok", LV_SYMBOL_OK}, {"close", LV_SYMBOL_CLOSE}, {"power", LV_SYMBOL_POWER},
    {"settings", LV_SYMBOL_SETTINGS}, {"home", LV_SYMBOL_HOME}, {"download", LV_SYMBOL_DOWNLOAD},
    {"drive", LV_SYMBOL_DRIVE}, {"refresh", LV_SYMBOL_REFRESH}, {"mute", LV_SYMBOL_MUTE},
    {"volume_mid", LV_SYMBOL_VOLUME_MID}, {"volume_max", LV_SYMBOL_VOLUME_MAX},
    {"image", LV_SYMBOL_IMAGE}, {"tint", LV_SYMBOL_TINT}, {"prev", LV_SYMBOL_PREV},
    {"play", LV_SYMBOL_PLAY}, {"pause", LV_SYMBOL_PAUSE}, {"stop", LV_SYMBOL_STOP},
    {"next", LV_SYMBOL_NEXT}, {"eject", LV_SYMBOL_EJECT}, {"left", LV_SYMBOL_LEFT},
    {"right", LV_SYMBOL_RIGHT}, {"plus", LV_SYMBOL_PLUS}, {"minus", LV_SYMBOL_MINUS},
    {"eye_open", LV_SYMBOL_EYE_OPEN}, {"eye_close", LV_SYMBOL_EYE_CLOSE},
    {"warning", LV_SYMBOL_WARNING}, {"shuffle", LV_SYMBOL_SHUFFLE}, {"up", LV_SYMBOL_UP},
    {"down", LV_SYMBOL_DOWN}, {"loop", LV_SYMBOL_LOOP}, {"directory", LV_SYMBOL_DIRECTORY},
    {"upload", LV_SYMBOL_UPLOAD}, {"call", LV_SYMBOL_CALL}, {"cut", LV_SYMBOL_CUT},
    {"copy", LV_SYMBOL_COPY}, {"save", LV_SYMBOL_SAVE}, {"bars", LV_SYMBOL_BARS},
    {"envelope", LV_SYMBOL_ENVELOPE}, {"charge", LV_SYMBOL_CHARGE}, {"paste", LV_SYMBOL_PASTE},
    {"bell", LV_SYMBOL_BELL}, {"keyboard", LV_SYMBOL_KEYBOARD}, {"gps", LV_SYMBOL_GPS},
    {"file", LV_SYMBOL_FILE}, {"wifi", LV_SYMBOL_WIFI}, {"battery", LV_SYMBOL_BATTERY_FULL},
    {"usb", LV_SYMBOL_USB}, {"bluetooth", LV_SYMBOL_BLUETOOTH}, {"trash", LV_SYMBOL_TRASH},
    {"edit", LV_SYMBOL_EDIT}, {"backspace", LV_SYMBOL_BACKSPACE}, {"sd_card", LV_SYMBOL_SD_CARD},
    {"new_line", LV_SYMBOL_NEW_LINE},
};

static inline const char *deckIconSymbol(const char *name) {
    if (!name || !*name) return nullptr;
    for (const DeckIcon &i : DECK_ICONS) {
        if (strcmp(i.name, name) == 0) return i.symbol;
    }
    return nullptr;
}
