// LongDeck – Stream-Deck-Firmware für den LilyGO T-Display-S3-Long
// https://github.com/joop0810/LongDeck
#include <Arduino.h>
#include "lvgl.h"
#include "pins_config.h"
#include "AXS15231B.h"
#include "hyn_core.h"
#include "config.h"
#include "keys.h"
#include "actions.h"
#include "net.h"
#include "ui.h"
#include "log.h"

// Wird vom LilyGO-Displaytreiber referenziert
void my_print(const char *buf) { LOGF("%s", buf); }

static lv_disp_draw_buf_t s_drawBuf;
static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;
static SemaphoreHandle_t s_touchMutex = nullptr;

static uint32_t s_lastActivity = 0;
static bool s_dimmed = false;
static bool s_swallowTouch = false;  // erste Berührung nach dem Dimmen nur zum Aufwecken

extern uint32_t transfer_num;
extern size_t lcd_PushColors_len;

// ---------------------------------------------------------------------------
// Hintergrundbeleuchtung (PWM)
// ---------------------------------------------------------------------------
static const int BL_CHANNEL = 0;

void backlightSet(uint8_t level) { ledcWrite(BL_CHANNEL, level); }

static void backlightBegin() {
    ledcSetup(BL_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, BL_CHANNEL);
    backlightSet(0);
}

static void activity() {
    s_lastActivity = millis();
    if (s_dimmed) {
        s_dimmed = false;
        backlightSet(g_config.brightness);
    }
}

// ---------------------------------------------------------------------------
// LVGL-Anbindung (wie im LilyGO-Beispiel: Software-Rotation, Full-Refresh, DMA)
// ---------------------------------------------------------------------------
static void dispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    while (get_lcd_spi_dma_write()) {
        lcd_PushColors(0, 0, 0, 0, NULL);
    }
    lcd_PushColors(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);
    // lv_disp_flush_ready() wird im DMA-Callback des Treibers aufgerufen
}

static void touchRead(lv_indev_drv_t *, lv_indev_data_t *data) {
    xSemaphoreTake(s_touchMutex, portMAX_DELAY);
    struct hyn_ts_data *ts = get_touch_data();
    bool pressed = ts && ts->rp_buf.rep_num > 0;
    if (pressed) {
        if (s_dimmed) s_swallowTouch = true;
        activity();
        if (s_swallowTouch) {
            data->state = LV_INDEV_STATE_REL;
        } else {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = ts->rp_buf.pos_info[0].pos_x;
            data->point.y = ts->rp_buf.pos_info[0].pos_y;
        }
    } else {
        s_swallowTouch = false;
        data->state = LV_INDEV_STATE_REL;
    }
    xSemaphoreGive(s_touchMutex);
}

static void lvglBegin() {
    lv_init();
    size_t px = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES;
    s_buf1 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * px);
    s_buf2 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * px);
    if (!s_buf1 || !s_buf2) {
        for (;;) {
            LOGF("PSRAM-Puffer konnte nicht angelegt werden\n");
            delay(1000);
        }
    }
    lv_disp_draw_buf_init(&s_drawBuf, s_buf1, s_buf2, px);

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = EXAMPLE_LCD_H_RES;
    dispDrv.ver_res = EXAMPLE_LCD_V_RES;
    dispDrv.flush_cb = dispFlush;
    dispDrv.draw_buf = &s_drawBuf;
    dispDrv.sw_rotate = 1;            // Querformat 640x180
    dispDrv.rotated = LV_DISP_ROT_90;
    dispDrv.full_refresh = 1;         // vom Treiber vorausgesetzt
    lv_disp_drv_register(&dispDrv);

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchRead;
    lv_indev_drv_register(&indevDrv);
}

// ---------------------------------------------------------------------------
void setup() {
    // USB zuerst, damit der PC das Gerät schnell als Tastatur erkennt
    configBegin();
    keysBegin(g_config.deviceName.c_str());
    delay(200);
    LOGF("\nLongDeck " LONGDECK_VERSION "\n");

    s_touchMutex = xSemaphoreCreateMutex();
    backlightBegin();
    axs15231_init();
    touch_init(TP_I2C_SDA, TP_I2C_SCL, TP_IRQ, TP_RST);
    lvglBegin();

    actionsBegin();
    netBegin();
    uiBuild();
    lv_timer_handler();
    backlightSet(g_config.brightness);
    s_lastActivity = millis();
}

void loop() {
    // LVGL nur rendern, wenn der vorherige DMA-Transfer fertig ist
    if (transfer_num <= 0 && lcd_PushColors_len <= 0) lv_timer_handler();
    if (transfer_num <= 1 && lcd_PushColors_len > 0) lcd_PushColors(0, 0, 0, 0, NULL);

    netLoop();

    if (g_configChanged) {
        g_configChanged = false;
        uiBuild();
        activity();
        backlightSet(g_config.brightness);
    }

    ActionResult r;
    while (actionsPollResult(r)) {
        if (r.gotoPage >= 0) uiGotoPage(r.gotoPage);
        if (!r.ok) {
            uiFlashButton(r.page, r.button, false);
            uiToast(r.msg[0] ? r.msg : "Fehler", false);
        } else if (r.msg[0]) {
            uiToast(r.msg, true);
        }
    }

    static uint32_t lastInfo = 0;
    if (millis() - lastInfo > 1000) {
        lastInfo = millis();
        uiUpdateInfo();
    }

    // Automatisch dimmen
    if (!s_dimmed && g_config.dimAfter > 0 && millis() - s_lastActivity > g_config.dimAfter * 1000UL) {
        s_dimmed = true;
        backlightSet(g_config.dimBrightness);
    }

    delay(2);
}
