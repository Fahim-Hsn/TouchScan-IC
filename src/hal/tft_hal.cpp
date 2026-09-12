/**
 * tft_hal.cpp — TFT Display + Touch Implementation
 * ST7789 SPI + XPT2046 via Adafruit_ST7789, registered as LVGL display/input drivers
 */
#include "tft_hal.h"
#include "../config.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>

// --- Pin Definitions for Adafruit ---
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define SPI_SCK  12
#define SPI_MISO 13
#define SPI_MOSI 11
#define TOUCH_CS 7
#define TOUCH_IRQ 6

// ─── Global instances ─────────────────────────────────────────────────────
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ─── LVGL Draw Buffers (allocated in SRAM) ────────────────────────────────
static lv_color_t fb_buf[DISPLAY_WIDTH * 30]; // 30 lines buffer (faster flushes)
static lv_disp_draw_buf_t disp_draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

static lv_coord_t last_touch_x = 0;
static lv_coord_t last_touch_y = 0;

// ─── LVGL Flush Callback ──────────────────────────────────────────────────
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // Write pixels directly using Adafruit's writePixels
    tft.writePixels((uint16_t*)color_p, w * h, true, false);
    tft.endWrite();

    lv_disp_flush_ready(drv);
}

// ─── LVGL Touch Read Callback ─────────────────────────────────────────────
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        if (p.z > 150) { // Valid touch pressure
            // Use the calibrated mapping for landscape mode
            int screen_x = map(p.x, 3647, 297, 0, DISPLAY_WIDTH - 1);
            int screen_y = map(p.y, 3770, 397, 0, DISPLAY_HEIGHT - 1);

            screen_x = constrain(screen_x, 0, DISPLAY_WIDTH - 1);
            screen_y = constrain(screen_y, 0, DISPLAY_HEIGHT - 1);

            last_touch_x = (lv_coord_t)screen_x;
            last_touch_y = (lv_coord_t)screen_y;

            data->point.x = last_touch_x;
            data->point.y = last_touch_y;
            data->state   = LV_INDEV_STATE_PRESSED;
            return;
        }
    }

    // Critical for LVGL: On release, maintain the last pressed point
    data->point.x = last_touch_x;
    data->point.y = last_touch_y;
    data->state   = LV_INDEV_STATE_RELEASED;
}

// ─── Public API ──────────────────────────────────────────────────────────
void tft_hal_init(void) {
    // Deselect both SPI devices before bus initialization
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    // --- Init SPI and Display ---
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    tft.init(240, 320);
    tft.setSPISpeed(40000000); // 40MHz SPI clock for ST7789
    tft.setRotation(1); // Landscape
    tft.fillScreen(ST77XX_BLACK);

    // --- Init Touch ---
    if (!ts.begin(SPI)) {
        Serial.println("[TFT] Touch initialization failed!");
    }
    ts.setRotation(1);

    // --- Backlight via LEDC ---
    ledcSetup(BL_LEDC_CH, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(TFT_BL_GPIO, BL_LEDC_CH);
    tft_hal_set_brightness(90);

    // --- Register LVGL display driver ---
    lv_disp_draw_buf_init(&disp_draw_buf, fb_buf, nullptr, DISPLAY_WIDTH * 30);
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = DISPLAY_WIDTH;
    disp_drv.ver_res    = DISPLAY_HEIGHT;
    disp_drv.flush_cb   = lvgl_flush_cb;
    disp_drv.draw_buf   = &disp_draw_buf;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);

    // --- Register LVGL touch input driver ---
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    Serial.println("[TFT] Adafruit Display + touch initialised");
}

void tft_hal_set_brightness(uint8_t pct) {
    if (pct > 100) pct = 100;
    uint32_t duty = (pct * 255) / 100;
    ledcWrite(BL_LEDC_CH, duty);
}

void tft_hal_calibrate_touch(void) {
    // Calibration is now hardcoded via the user's working map() values in the touch callback.
    Serial.println("[TFT] Touch is pre-calibrated.");
}
