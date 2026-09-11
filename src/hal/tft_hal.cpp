/**
 * tft_hal.cpp — TFT Display + Touch Implementation
 * ST7789 SPI + XPT2046 via TFT_eSPI, registered as LVGL display/input drivers
 */
#include "tft_hal.h"
#include "../config.h"

// ─── Global TFT instance ──────────────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();

// ─── LVGL Draw Buffers (allocated in PSRAM) ───────────────────────────────
// Full double-buffer for maximum smoothness (320*240*2 bytes each = 150 KB)
static lv_color_t *draw_buf1 = nullptr;
static lv_color_t *draw_buf2 = nullptr;

static lv_disp_draw_buf_t disp_draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

// ─── LVGL Flush Callback ──────────────────────────────────────────────────
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // pushColors casts lv_color_t* → uint16_t* (RGB565 direct)
    tft.pushColors(reinterpret_cast<uint16_t *>(color_p), w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(drv);
}

// ─── LVGL Touch Read Callback ─────────────────────────────────────────────
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t tx = 0, ty = 0;
    if (tft.getTouch(&tx, &ty, 600)) {   // 600 = Z-threshold (adjust if needed)
        data->point.x  = (lv_coord_t)tx;
        data->point.y  = (lv_coord_t)ty;
        data->state    = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ─── Public API ──────────────────────────────────────────────────────────
void tft_hal_init(void) {
    // --- Allocate draw buffers in PSRAM ---
    size_t buf_px = DISPLAY_WIDTH * DISPLAY_HEIGHT;
    draw_buf1 = static_cast<lv_color_t *>(ps_malloc(buf_px * sizeof(lv_color_t)));
    draw_buf2 = static_cast<lv_color_t *>(ps_malloc(buf_px * sizeof(lv_color_t)));

    if (!draw_buf1 || !draw_buf2) {
        Serial.println("[TFT] ERROR: PSRAM allocation failed for draw buffers!");
        // Fallback to a smaller buffer in DRAM
        static lv_color_t fb_small[320 * 10];
        lv_disp_draw_buf_init(&disp_draw_buf, fb_small, nullptr, 320 * 10);
    } else {
        lv_disp_draw_buf_init(&disp_draw_buf, draw_buf1, draw_buf2, buf_px);
    }

    // --- Init TFT hardware ---
    tft.init();
    tft.setRotation(1);          // Landscape: 320 x 240
    tft.fillScreen(TFT_BLACK);

    // --- Backlight via LEDC ---
    ledcSetup(BL_LEDC_CH, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(TFT_BL_GPIO, BL_LEDC_CH);
    tft_hal_set_brightness(90);  // 90% brightness default

    // --- Apply touch calibration ---
    tft.setTouch(const_cast<uint16_t *>(TOUCH_CAL));

    // --- Register LVGL display driver ---
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = DISPLAY_WIDTH;
    disp_drv.ver_res    = DISPLAY_HEIGHT;
    disp_drv.flush_cb   = lvgl_flush_cb;
    disp_drv.draw_buf   = &disp_draw_buf;
    disp_drv.full_refresh = 0;  // Partial refresh enabled
    lv_disp_drv_register(&disp_drv);

    // --- Register LVGL touch input driver ---
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    Serial.println("[TFT] Display + touch initialised (320x240 landscape)");
}

void tft_hal_set_brightness(uint8_t pct) {
    if (pct > 100) pct = 100;
    uint32_t duty = (pct * 255) / 100;
    ledcWrite(BL_LEDC_CH, duty);
}

void tft_hal_calibrate_touch(void) {
    uint16_t cal[5];
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.println("Touch calibration — follow the dots");
    tft.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 15);
    // Print calibration values for user to paste into config.h
    Serial.print("[TFT] Calibration values: { ");
    for (int i = 0; i < 5; i++) {
        Serial.print(cal[i]);
        if (i < 4) Serial.print(", ");
    }
    Serial.println(" }");
    // Apply immediately
    tft.setTouch(cal);
}
