/**
 * tft_hal.h — TFT Display + Touch Hardware Abstraction Layer
 * ST7789 (SPI) + XPT2046 touch via Adafruit_ST7789
 */
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <lvgl.h>

// Exported TFT instance
extern Adafruit_ST7789 tft;

/**
 * @brief Initialise TFT display, register LVGL display & input drivers.
 *        Must be called after lv_init() and before any lv_ UI calls.
 */
void tft_hal_init(void);

/**
 * @brief Set display backlight brightness.
 * @param pct  0–100 (%)
 */
void tft_hal_set_brightness(uint8_t pct);

/**
 * @brief Set runtime touch calibration parameters.
 */
void tft_hal_set_calibration(int cal_x_min, int cal_x_max, int cal_y_min, int cal_y_max);

/**
 * @brief Get raw unmapped touch reading from XPT2046.
 * @return true if currently touched, false otherwise.
 */
bool tft_hal_get_raw_touch(int *raw_x, int *raw_y);

/**
 * @brief Launch touch calibration UI.
 */
void tft_hal_calibrate_touch(void);
