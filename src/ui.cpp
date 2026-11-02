#include "ui.h"
#include "config.h"
#include "actions.h"
#include "protocol.h"
#include "keys.h"
#include "icons.h"
#include "lvgl.h"
#include <vector>

// Schriften mit Umlauten, Sonderzeichen (Latin-1, €, „“) und LVGL-Symbolen (src/font_de_*.c)
LV_FONT_DECLARE(font_de_14);
LV_FONT_DECLARE(font_de_20);
LV_FONT_DECLARE(font_de_28);

extern void backlightSet(uint8_t level);  // main.cpp

static const int SCR_W = 640;
static const int SCR_H = 180;
static const int PAD = 6;
static const int GAP = 6;
static const int DOTS_H = 12;

static lv_obj_t *s_tv = nullptr;
static lv_obj_t *s_dots = nullptr;
static lv_obj_t *s_toast = nullptr;
static lv_timer_t *s_toastTimer = nullptr;
static std::vector<std::vector<lv_obj_t *>> s_btns;
static int s_pageCount = 0;

// Info-Seite
static lv_obj_t *s_infoUsb = nullptr;
static lv_obj_t *s_infoEditor = nullptr;
static int s_lastUsb = -1, s_lastEd = -1;
static lv_obj_t *s_slider = nullptr;

static lv_color_t hex(uint32_t c) { return lv_color_hex(c); }

// ---------------------------------------------------------------------------
static void updateDots() {
    if (!s_dots || !s_tv) return;
    lv_obj_t *act = lv_tileview_get_tile_act(s_tv);
    int idx = act ? lv_obj_get_index(act) : 0;
    uint32_t n = lv_obj_get_child_cnt(s_dots);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *d = lv_obj_get_child(s_dots, i);
        bool on = (int)i == idx;
        lv_obj_set_size(d, on ? 18 : 6, 6);
        lv_obj_set_style_bg_color(d, on ? hex(0xFFFFFF) : hex(0x555A66), 0);
    }
}

static void onTileChanged(lv_event_t *) { updateDots(); }

static void onButtonClicked(lv_event_t *e) {
    uint32_t data = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    actionsTrigger((int)(data >> 8), (int)(data & 0xFF));
}

// ---------------------------------------------------------------------------
// Hintergrund für das Glas-Design: dunkler Verlauf mit weichen Farbflecken,
// einmalig in den PSRAM gerechnet (LVGL 8 kann nicht weichzeichnen).
// ---------------------------------------------------------------------------
static lv_img_dsc_t s_backdropDsc;
static lv_color_t *s_backdropBuf = nullptr;
static lv_obj_t *s_backdrop = nullptr;

struct Blob { float x, y, r; uint8_t cr, cg, cb; float strength; };

static void renderBackdrop() {
    if (s_backdropBuf) return;
    s_backdropBuf = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * SCR_W * SCR_H);
    if (!s_backdropBuf) return;
    const Blob blobs[] = {
        {90, 10, 260, 0x2B, 0x59, 0xF0, 0.45f},   // Blau oben links
        {560, 170, 280, 0x8A, 0x3F, 0xFC, 0.42f},  // Violett unten rechts
        {330, 190, 200, 0x10, 0xC8, 0xB0, 0.25f},  // Türkis unten Mitte
    };
    for (int y = 0; y < SCR_H; y++) {
        // Grundverlauf: fast schwarz, unten minimal heller
        float t = (float)y / SCR_H;
        float br = 6 + 6 * t, bgc = 7 + 7 * t, bb = 11 + 10 * t;
        for (int x = 0; x < SCR_W; x++) {
            float r = br, g = bgc, b = bb;
            for (const Blob &bl : blobs) {
                float dx = x - bl.x, dy = (y - bl.y) * 1.6f;
                float d2 = (dx * dx + dy * dy) / (bl.r * bl.r);
                if (d2 >= 1.0f) continue;
                float f = (1.0f - d2);
                f = f * f * bl.strength;  // weicher Abfall
                r += bl.cr * f;
                g += bl.cg * f;
                b += bl.cb * f;
            }
            // leichtes Dithering gegen Farbstufen
            float n = ((x * 7 + y * 13) & 3) * 0.6f;
            s_backdropBuf[y * SCR_W + x] = lv_color_make((uint8_t)min(255.0f, r + n), (uint8_t)min(255.0f, g + n),
                                                         (uint8_t)min(255.0f, b + n));
        }
    }
    s_backdropDsc.header.always_zero = 0;
    s_backdropDsc.header.w = SCR_W;
    s_backdropDsc.header.h = SCR_H;
    s_backdropDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_backdropDsc.data_size = sizeof(lv_color_t) * SCR_W * SCR_H;
    s_backdropDsc.data = (const uint8_t *)s_backdropBuf;
}

static void styleGlass(lv_obj_t *o, lv_color_t c, int w, int h) {
    lv_color_t black = lv_color_black();
    lv_color_t white = lv_color_white();
    lv_obj_set_style_bg_opa(o, 165, 0);
    lv_obj_set_style_bg_color(o, lv_color_mix(c, black, 165), 0);
    lv_obj_set_style_bg_grad_color(o, lv_color_mix(c, black, 70), 0);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_border_color(o, lv_color_mix(white, c, 150), 0);
    lv_obj_set_style_border_opa(o, 120, 0);
    lv_obj_set_style_shadow_color(o, c, 0);
    lv_obj_set_style_shadow_width(o, 22, 0);
    lv_obj_set_style_shadow_opa(o, 95, 0);
    lv_obj_set_style_shadow_ofs_y(o, 3, 0);
    lv_obj_set_style_shadow_spread(o, -3, 0);
    // gedrückt: heller, Kante leuchtet
    lv_obj_set_style_bg_opa(o, 225, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(o, lv_color_mix(c, black, 220), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(o, lv_color_mix(c, black, 130), LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(o, 255, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(o, 180, LV_STATE_PRESSED);
    lv_obj_set_style_transform_width(o, -3, LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(o, -3, LV_STATE_PRESSED);
    lv_obj_set_style_clip_corner(o, true, 0);

    // Glanz: heller Streifen in der oberen Hälfte
    lv_obj_t *sheen = lv_obj_create(o);
    lv_obj_remove_style_all(sheen);
    lv_obj_add_flag(sheen, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(sheen, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(sheen, w - 2, h * 45 / 100);
    lv_obj_set_pos(sheen, 1, 1);
    lv_obj_set_style_radius(sheen, 13, 0);
    lv_obj_set_style_bg_opa(sheen, 28, 0);
    lv_obj_set_style_bg_color(sheen, white, 0);
    lv_obj_set_style_bg_grad_color(sheen, c, 0);
    lv_obj_set_style_bg_grad_dir(sheen, LV_GRAD_DIR_VER, 0);
}

// ---------------------------------------------------------------------------
// Skins
// ---------------------------------------------------------------------------
enum Skin { SKIN_MINIMAL, SKIN_SOFT, SKIN_BOLD, SKIN_MONO, SKIN_GLASS, SKIN_FLAT,
            SKIN_FROST, SKIN_FROST_TINT, SKIN_FROST_LIGHT, SKIN_FROST_DARK };
static bool skinHasBackdrop(int s) { return s == SKIN_GLASS || s >= SKIN_FROST; }

static Skin currentSkin() {
    const String &t = g_config.theme;
    if (t == "soft") return SKIN_SOFT;
    if (t == "bold") return SKIN_BOLD;
    if (t == "mono") return SKIN_MONO;
    if (t == "glass") return SKIN_GLASS;
    if (t == "frost") return SKIN_FROST;
    if (t == "frost-tint") return SKIN_FROST_TINT;
    if (t == "frost-light") return SKIN_FROST_LIGHT;
    if (t == "frost-dark") return SKIN_FROST_DARK;
    if (t == "flat") return SKIN_FLAT;
    return SKIN_MINIMAL;
}

// Farbe so aufhellen, dass sie als Akzent auf Schwarz gut leuchtet
static lv_color_t accentOf(lv_color_t c) {
    uint32_t c32 = lv_color_to32(c);
    int r = (c32 >> 16) & 0xFF, g = (c32 >> 8) & 0xFF, b = c32 & 0xFF;
    int mx = max(r, max(g, b));
    if (mx < 30) return lv_color_hex(0xD4D4D8);  // fast schwarz -> neutrales Hellgrau
    int mn0 = min(r, min(g, b));
    if ((mx - mn0) * 10 < mx * 3) return lv_color_hex(0xC8C8CE);  // kaum Farbe -> neutrales Grau
    // Sättigung erhalten, Helligkeit auf ~ 235 anheben
    float k = 235.0f / mx;
    if (k < 1.0f) k = 1.0f;
    r = min(255, (int)(r * k)); g = min(255, (int)(g * k)); b = min(255, (int)(b * k));
    // graue Töne (wenig Sättigung) nicht grell machen
    int mn = min(r, min(g, b));
    if (max(r, max(g, b)) - mn < 40) { r = g = b = 200; }
    return lv_color_make(r, g, b);
}

static void createButton(lv_obj_t *parent, const DeckButton &b, int page, int idx, int x, int y, int w, int h) {
    Skin skin = currentSkin();
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    int radius = (skin == SKIN_SOFT || skin == SKIN_BOLD) ? 18 : (skin == SKIN_MINIMAL ? 16 : 14);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_user_data(o, (void *)(uintptr_t)0);

    bool empty = b.type == ACT_NONE && b.label.length() == 0 && b.icon.length() == 0;
    if (empty) {
        if (skinHasBackdrop(skin)) {
            lv_obj_set_style_bg_color(o, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(o, 8, 0);
            lv_obj_set_style_border_color(o, lv_color_white(), 0);
            lv_obj_set_style_border_opa(o, 18, 0);
        } else {
            lv_obj_set_style_bg_color(o, hex(0x0A0A0C), 0);
            lv_obj_set_style_border_color(o, hex(0x1A1A1F), 0);
        }
        lv_obj_set_style_border_width(o, 1, 0);
        lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
        s_btns[page].push_back(o);
        return;
    }

    lv_color_t base = hex(b.bg);
    lv_color_t acc = accentOf(base);
    lv_color_t fg = hex(b.fg);
    lv_color_t black = lv_color_black();
    lv_color_t iconColor = fg, labelColor = fg;
    bool preferFilled = false, showLabel = true;
    uint32_t edge = 0;  // Randfarbe für uiFlashButton (0 = kein Rand)

    switch (skin) {
        case SKIN_MINIMAL:
            lv_obj_set_style_bg_color(o, hex(0x111114), 0);
            lv_obj_set_style_border_width(o, 1, 0);
            lv_obj_set_style_border_color(o, hex(0x232329), 0);
            lv_obj_set_style_bg_color(o, hex(0x1C1C22), LV_STATE_PRESSED);
            lv_obj_set_style_border_color(o, acc, LV_STATE_PRESSED);
            iconColor = acc;
            labelColor = hex(0x9A9AA3);
            edge = 0xFF232329;
            break;
        case SKIN_SOFT:
            lv_obj_set_style_bg_color(o, lv_color_mix(acc, black, 38), 0);
            lv_obj_set_style_bg_color(o, lv_color_mix(acc, black, 75), LV_STATE_PRESSED);
            iconColor = acc;
            labelColor = hex(0xE7E7EA);
            preferFilled = true;
            break;
        case SKIN_BOLD:
            // graue Tasten dunkel halten, sonst ist das weiße Icon kaum lesbar
            if (lv_color_to32(acc) == lv_color_to32(lv_color_hex(0xC8C8CE)) ||
                lv_color_to32(acc) == lv_color_to32(lv_color_hex(0xD4D4D8))) acc = hex(0x3F3F46);
            lv_obj_set_style_bg_color(o, acc, 0);
            lv_obj_set_style_bg_color(o, lv_color_mix(lv_color_white(), acc, 60), LV_STATE_PRESSED);
            iconColor = lv_color_white();
            labelColor = lv_color_white();
            preferFilled = true;
            showLabel = b.icon.length() == 0;  // nur Icon, Text nur wenn kein Icon gewählt
            break;
        case SKIN_MONO:
            lv_obj_set_style_bg_color(o, hex(0x161618), 0);
            lv_obj_set_style_bg_color(o, hex(0x26262A), LV_STATE_PRESSED);
            iconColor = hex(0xF4F4F5);
            labelColor = hex(0x71717A);
            break;
        case SKIN_GLASS: {
            styleGlass(o, base, w, h);
            edge = (120u << 24) | (lv_color_to32(lv_color_mix(lv_color_white(), base, 150)) & 0xFFFFFF);
            break;
        }
        case SKIN_FROST:        // Milchglas: neutral, Icon in Farbe
        case SKIN_FROST_TINT:   // Milchglas in Tastenfarbe getönt, Icon weiß
        case SKIN_FROST_LIGHT:  // helles Glas, schwarze Icons
        case SKIN_FROST_DARK: { // dunkles Glas, weiße Icons
            lv_color_t white = lv_color_white();
            lv_color_t fill = white;
            uint8_t opa = 22, opaP = 45, bOpa = 26;
            lv_color_t bcol = white;
            if (skin == SKIN_FROST) { iconColor = acc; labelColor = hex(0xC9CBD1); }
            if (skin == SKIN_FROST_TINT) { fill = acc; opa = 46; opaP = 90; bcol = acc; bOpa = 60; iconColor = white; labelColor = hex(0xEDEEF2); }
            if (skin == SKIN_FROST_LIGHT) { fill = hex(0xF4F5F8); opa = 205; opaP = 245; bOpa = 0; iconColor = hex(0x111216); labelColor = hex(0x4A4D55); preferFilled = true; }
            if (skin == SKIN_FROST_DARK) { fill = lv_color_black(); opa = 120; opaP = 170; bOpa = 20; iconColor = hex(0xF4F4F5); labelColor = hex(0x9A9CA5); }
            lv_obj_set_style_bg_color(o, fill, 0);
            lv_obj_set_style_bg_opa(o, opa, 0);
            lv_obj_set_style_bg_opa(o, opaP, LV_STATE_PRESSED);
            if (bOpa) {
                lv_obj_set_style_border_width(o, 1, 0);
                lv_obj_set_style_border_color(o, bcol, 0);
                lv_obj_set_style_border_opa(o, bOpa, 0);
                lv_obj_set_style_border_opa(o, min(255, bOpa * 3), LV_STATE_PRESSED);
                edge = ((uint32_t)bOpa << 24) | (lv_color_to32(bcol) & 0xFFFFFF);
            }
            break;
        }
        case SKIN_FLAT:
        default:
            lv_obj_set_style_bg_color(o, base, 0);
            lv_obj_set_style_bg_grad_color(o, lv_color_darken(base, 40), 0);
            lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
            lv_obj_set_style_bg_color(o, lv_color_lighten(base, 70), LV_STATE_PRESSED);
            lv_obj_set_style_bg_grad_color(o, lv_color_lighten(base, 30), LV_STATE_PRESSED);
            break;
    }
    if (skin != SKIN_GLASS) {
        lv_obj_set_style_transform_width(o, -2, LV_STATE_PRESSED);
        lv_obj_set_style_transform_height(o, -2, LV_STATE_PRESSED);
    }
    lv_obj_set_user_data(o, (void *)(uintptr_t)edge);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(o, 4, 0);
    lv_obj_set_style_pad_all(o, 4, 0);

    // Icon (bei manchen Skins bevorzugt gefüllt)
    const DeckIcon *dic = nullptr;
    if (b.icon.length()) {
        if (preferFilled && !b.icon.endsWith("-filled")) dic = deckIconFind((b.icon + "-filled").c_str());
        if (!dic) dic = deckIconFind(b.icon.c_str());
    }
    bool hasLabel = showLabel && b.label.length();
    if (dic) {
        lv_obj_t *ic = lv_label_create(o);
        lv_label_set_text(ic, dic->glyph);
        int room = min(w, h) - (hasLabel ? 22 : 0);
        int px = room >= 90 ? 48 : (room >= 48 ? 32 : 24);
        if (skin == SKIN_BOLD && !hasLabel && room >= 70) px = 48;
        lv_obj_set_style_text_font(ic, deckIconFont(dic, px), 0);
        lv_obj_set_style_text_color(ic, iconColor, 0);
        lv_obj_clear_flag(ic, LV_OBJ_FLAG_CLICKABLE);
    }
    if (hasLabel || (!dic && b.label.length())) {
        lv_obj_t *lb = lv_label_create(o);
        lv_label_set_text(lb, b.label.c_str());
        bool big = !dic && b.label.length() <= 8 && b.label.indexOf('\n') < 0 && w >= 80;
        lv_obj_set_style_text_font(lb, big ? &font_de_20 : &font_de_14, 0);
        lv_obj_set_style_text_color(lb, dic ? labelColor : iconColor, 0);
        lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(lb, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lb, w - 8);
        lv_obj_clear_flag(lb, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_add_event_cb(o, onButtonClicked, LV_EVENT_CLICKED, (void *)(uintptr_t)((page << 8) | idx));
    s_btns[page].push_back(o);
}

static void onRestartBtn(lv_event_t *) { ESP.restart(); }

static void onSlider(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int v = lv_slider_get_value(s);
    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    g_config.brightness = v;
    xSemaphoreGiveRecursive(g_configMutex);
    backlightSet(v);
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
        configSave();
        xSemaphoreGiveRecursive(g_configMutex);
    }
}

static lv_obj_t *mkLabel(lv_obj_t *parent, const lv_font_t *font, uint32_t color, int x, int y, int w) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, hex(color), 0);
    lv_obj_set_pos(l, x, y);
    if (w > 0) {
        lv_obj_set_width(l, w);
        lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    }
    lv_label_set_text(l, "");
    return l;
}

static lv_obj_t *mkSmallBtn(lv_obj_t *parent, const char *text, int x, int y, lv_event_cb_t cb) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, 150, 40);
    lv_obj_set_style_bg_color(b, hex(0x2A2F3A), 0);
    lv_obj_set_style_bg_color(b, hex(0x4A5160), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &font_de_14, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    return b;
}

static void buildInfoTile(lv_obj_t *tile) {
    lv_obj_t *t = mkLabel(tile, &font_de_28, 0xFFFFFF, 16, 12, 0);
    lv_label_set_text(t, LV_SYMBOL_KEYBOARD " LongDeck");
    lv_obj_t *v = mkLabel(tile, &font_de_14, 0x8A90A0, 18, 48, 0);
    lv_label_set_text(v, "Version " LONGDECK_VERSION);

    s_infoUsb = mkLabel(tile, &font_de_14, 0xFFFFFF, 18, 78, 430);
    s_infoEditor = mkLabel(tile, &font_de_14, 0x8A90A0, 18, 100, 430);
    lv_obj_t *h1 = mkLabel(tile, &font_de_14, 0x8A90A0, 18, 130, 430);
    lv_label_set_text(h1, "Tasten belegen (Chrome/Edge):");
    lv_obj_t *h2 = mkLabel(tile, &font_de_14, 0x5EC8FF, 18, 150, 440);
    lv_label_set_text(h2, "joop0810.github.io/LongDeck");

    mkSmallBtn(tile, LV_SYMBOL_REFRESH " Neustart", 472, 12, onRestartBtn);

    lv_obj_t *sl = lv_slider_create(tile);
    lv_obj_set_pos(sl, 478, 128);
    lv_obj_set_size(sl, 138, 14);
    lv_slider_set_range(sl, 10, 255);
    lv_slider_set_value(sl, g_config.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, hex(0x2A2F3A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, hex(0x5EC8FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_add_event_cb(sl, onSlider, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(sl, onSlider, LV_EVENT_RELEASED, nullptr);
    s_slider = sl;
    lv_obj_t *bl = mkLabel(tile, &font_de_14, 0x8A90A0, 478, 100, 0);
    lv_label_set_text(bl, LV_SYMBOL_IMAGE " Helligkeit");
    uiUpdateInfo();
}

void uiUpdateInfo() {
    if (!s_infoUsb) return;
    int usb = keysUsbReady() ? 1 : 0;
    int ed = protocolEditorConnected() ? 1 : 0;
    if (usb == s_lastUsb && ed == s_lastEd) return;  // nur bei Änderung neu zeichnen
    s_lastUsb = usb;
    s_lastEd = ed;
    lv_label_set_text(s_infoUsb, usb ? LV_SYMBOL_USB "  Mit PC verbunden – Tastatur bereit"
                                     : LV_SYMBOL_USB "  Kein PC erkannt (nur Strom?)");
    lv_obj_set_style_text_color(s_infoUsb, usb ? hex(0x4ADE80) : hex(0xFFC857), 0);
    lv_label_set_text(s_infoEditor, ed ? LV_SYMBOL_EDIT "  Editor verbunden" : "");
}

// ---------------------------------------------------------------------------
void uiBuild() {
    int keepPage = 0;
    if (s_tv) {
        lv_obj_t *act = lv_tileview_get_tile_act(s_tv);
        keepPage = act ? lv_obj_get_index(act) : 0;
        lv_obj_del(s_tv);
        s_tv = nullptr;
    }
    if (s_dots) {
        lv_obj_del(s_dots);
        s_dots = nullptr;
    }
    s_infoUsb = s_infoEditor = s_slider = nullptr;
    s_lastUsb = s_lastEd = -1;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, hex(0x000000), 0);
    bool glassTheme = skinHasBackdrop(currentSkin());
    if (glassTheme) renderBackdrop();
    if (glassTheme && s_backdropBuf) {
        if (!s_backdrop) {
            s_backdrop = lv_img_create(scr);
            lv_img_set_src(s_backdrop, &s_backdropDsc);
            lv_obj_set_pos(s_backdrop, 0, 0);
            lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_CLICKABLE);
        }
        lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(s_backdrop);
    } else if (s_backdrop) {
        lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_text_font(scr, &font_de_14, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    const DeckConfig &c = g_config;
    s_pageCount = c.pages.size();
    s_btns.assign(s_pageCount, {});

    s_tv = lv_tileview_create(scr);
    lv_obj_set_size(s_tv, SCR_W, SCR_H);
    lv_obj_set_style_bg_color(s_tv, hex(currentSkin() == SKIN_BOLD ? 0x0A0A0B : 0x000000), 0);
    lv_obj_set_style_bg_opa(s_tv, glassTheme ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(s_tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(s_tv, onTileChanged, LV_EVENT_VALUE_CHANGED, nullptr);

    int cols = c.cols, rows = c.rows;
    int w = (SCR_W - 2 * PAD - (cols - 1) * GAP) / cols;
    int availH = SCR_H - PAD - DOTS_H;
    int h = (availH - (rows - 1) * GAP) / rows;
    if (c.squareTiles && h > w) h = w;  // nicht langziehen – stattdessen Rand oben/unten
    int totalW = cols * w + (cols - 1) * GAP;
    int totalH = rows * h + (rows - 1) * GAP;
    int x0 = (SCR_W - totalW) / 2;
    int y0 = PAD + (availH - totalH) / 2;

    for (int p = 0; p < s_pageCount; p++) {
        lv_obj_t *tile = lv_tileview_add_tile(s_tv, p, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
        const DeckPage &page = c.pages[p];
        for (int r = 0; r < rows; r++) {
            for (int col = 0; col < cols; col++) {
                int idx = r * cols + col;
                if (idx >= (int)page.buttons.size()) continue;
                createButton(tile, page.buttons[idx], p, idx, x0 + col * (w + GAP), y0 + r * (h + GAP), w, h);
            }
        }
    }
    lv_obj_t *info = lv_tileview_add_tile(s_tv, s_pageCount, 0, LV_DIR_LEFT);
    buildInfoTile(info);
    xSemaphoreGiveRecursive(g_configMutex);

    // Seitenpunkte
    s_dots = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_dots);
    lv_obj_set_size(s_dots, LV_SIZE_CONTENT, 8);
    lv_obj_set_flex_flow(s_dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_dots, 6, 0);
    lv_obj_clear_flag(s_dots, LV_OBJ_FLAG_CLICKABLE);
    for (int i = 0; i <= s_pageCount; i++) {
        lv_obj_t *d = lv_obj_create(s_dots);
        lv_obj_remove_style_all(d);
        lv_obj_set_style_radius(d, 3, 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_size(d, 6, 6);
    }
    lv_obj_align(s_dots, LV_ALIGN_BOTTOM_MID, 0, -2);

    if (keepPage > s_pageCount) keepPage = s_pageCount;
    lv_obj_set_tile_id(s_tv, keepPage, 0, LV_ANIM_OFF);
    updateDots();
}

void uiGotoPage(int page) {
    if (!s_tv || page < 0 || page > s_pageCount) return;
    lv_obj_set_tile_id(s_tv, page, 0, LV_ANIM_ON);
    updateDots();
}

static void toastHide(lv_timer_t *t) {
    if (s_toast) lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    s_toastTimer = nullptr;
}

void uiToast(const char *msg, bool ok) {
    if (!s_toast) {
        s_toast = lv_label_create(lv_layer_top());
        lv_obj_set_style_bg_opa(s_toast, LV_OPA_90, 0);
        lv_obj_set_style_radius(s_toast, 10, 0);
        lv_obj_set_style_pad_hor(s_toast, 14, 0);
        lv_obj_set_style_pad_ver(s_toast, 8, 0);
        lv_obj_set_style_text_font(s_toast, &font_de_14, 0);
        lv_obj_set_style_text_color(s_toast, hex(0xFFFFFF), 0);
        lv_obj_align(s_toast, LV_ALIGN_TOP_MID, 0, 8);
    }
    lv_obj_set_style_bg_color(s_toast, ok ? hex(0x1E7A3C) : hex(0xA3262E), 0);
    lv_label_set_text(s_toast, msg);
    lv_obj_align(s_toast, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_toast);
    if (s_toastTimer) lv_timer_del(s_toastTimer);
    s_toastTimer = lv_timer_create(toastHide, ok ? 1200 : 2500, nullptr);
    lv_timer_set_repeat_count(s_toastTimer, 1);
}

static void flashOff(lv_timer_t *t) {
    lv_obj_t *o = (lv_obj_t *)t->user_data;
    // Objekt könnte inzwischen durch Neuaufbau gelöscht sein
    if (!lv_obj_is_valid(o)) return;
    uint32_t edge = (uint32_t)(uintptr_t)lv_obj_get_user_data(o);  // Bits 24-31: Deckkraft, 0-23: Farbe
    lv_obj_set_style_border_width(o, edge ? 1 : 0, 0);
    if (edge) {
        lv_obj_set_style_border_opa(o, edge >> 24, 0);
        lv_obj_set_style_border_color(o, lv_color_hex(edge & 0xFFFFFF), 0);
    }
}

void uiFlashButton(int page, int button, bool ok) {
    if (page < 0 || page >= (int)s_btns.size()) return;
    if (button < 0 || button >= (int)s_btns[page].size()) return;
    lv_obj_t *o = s_btns[page][button];
    lv_obj_set_style_border_color(o, ok ? hex(0x4ADE80) : hex(0xFF4D4F), 0);
    lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 3, 0);
    lv_timer_t *t = lv_timer_create(flashOff, 600, o);
    lv_timer_set_repeat_count(t, 1);
}
