#pragma once
#include "lvgl.h"
#include <string.h>
#include "icons_gen.h"  // Tabler Icons, erzeugt von tools/icons/gen_icons.py

// Icon-Schriften (Outline / Filled) in drei Größen
LV_FONT_DECLARE(icon_o_24);
LV_FONT_DECLARE(icon_o_32);
LV_FONT_DECLARE(icon_o_48);
LV_FONT_DECLARE(icon_f_24);
LV_FONT_DECLARE(icon_f_32);
LV_FONT_DECLARE(icon_f_48);

// Alte Symbolnamen (Version 1.0/1.1) -> Tabler-Namen, damit bestehende Belegungen weiter passen
struct IconAlias { const char *from; const char *to; };
static const IconAlias ICON_ALIASES[] = {
    {"audio", "music"}, {"video", "video"}, {"list", "list"}, {"ok", "check"}, {"close", "x"},
    {"power", "power"}, {"settings", "settings"}, {"home", "home"}, {"download", "download"},
    {"drive", "database"}, {"refresh", "arrow-forward-up"}, {"mute", "volume-off"},
    {"volume_mid", "volume-2"}, {"volume_max", "volume"}, {"image", "screenshot"}, {"tint", "droplet"},
    {"prev", "player-track-prev"}, {"play", "player-play"}, {"pause", "player-pause"},
    {"stop", "player-stop"}, {"next", "player-track-next"}, {"eject", "player-eject"},
    {"left", "arrow-left"}, {"right", "arrow-right"}, {"plus", "plus"}, {"minus", "minus"},
    {"eye_open", "eye"}, {"eye_close", "eye-off"}, {"warning", "alert-triangle"},
    {"shuffle", "switch-horizontal"}, {"up", "arrow-up"}, {"down", "arrow-down"},
    {"loop", "arrow-back-up"}, {"directory", "folder"}, {"upload", "upload"}, {"call", "phone"},
    {"cut", "cut"}, {"copy", "copy"}, {"save", "device-floppy"}, {"bars", "menu-2"},
    {"envelope", "mail"}, {"charge", "bolt"}, {"paste", "clipboard"}, {"bell", "bell"},
    {"keyboard", "keyboard"}, {"gps", "map-pin"}, {"file", "file"}, {"wifi", "wifi"},
    {"battery", "battery"}, {"usb", "usb"}, {"bluetooth", "bluetooth"}, {"trash", "trash"},
    {"edit", "pencil"}, {"backspace", "backspace"}, {"sd_card", "device-floppy"}, {"new_line", "corner-down-left"},
};

static inline const char *iconCanonicalName(const char *name) {
    if (!name || !*name) return name;
    for (const IconAlias &a : ICON_ALIASES) {
        if (strcmp(a.from, name) == 0) return a.to;
    }
    return name;
}

static inline const DeckIcon *deckIconFind(const char *name) {
    if (!name || !*name) return nullptr;
    name = iconCanonicalName(name);
    for (size_t i = 0; i < DECK_ICON_COUNT; i++) {
        if (strcmp(DECK_ICONS[i].name, name) == 0) return &DECK_ICONS[i];
    }
    return nullptr;
}

// px: gewünschte Größe (24, 32 oder 48)
static inline const lv_font_t *deckIconFont(const DeckIcon *ic, int px) {
    if (px >= 48) return ic->filled ? &icon_f_48 : &icon_o_48;
    if (px >= 32) return ic->filled ? &icon_f_32 : &icon_o_32;
    return ic->filled ? &icon_f_24 : &icon_o_24;
}
