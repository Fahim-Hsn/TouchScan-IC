/**
 * tft_hal.h — TFT Display + Touch Hardware Abstraction Layer
 * ST7789 (SPI) + XPT2046 touch via TFT_eSPI
 */
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

// Exported TFT instance (used by other modules for direct drawing during init)
extern TFT_eSPI tft;

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
 * @brief Run the XPT2046 touch calibration routine interactively.
 *        Draws crosshairs on screen and saves calibration data.
 *        Call once on first boot (or from Settings menu).
 */
void tft_hal_calibrate_touch(void);
