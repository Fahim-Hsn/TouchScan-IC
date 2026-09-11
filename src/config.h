/**
 * config.h — Master Pin & Constants Configuration
 * Digital IC Checker | ESP32-S3 N16R8
 *
 * ┌─────────────────────────────────────────────────┐
 * │            RECOMMENDED GPIO MAPPING             │
 * ├──────────────┬────────────┬────────────────────┤
 * │  PERIPHERAL  │  GPIO      │  NOTES              │
 * ├──────────────┼────────────┼────────────────────┤
 * │ TFT MOSI     │  GPIO 11   │ SPI2                │
 * │ TFT SCK      │  GPIO 12   │ SPI2                │
 * │ TFT MISO     │  GPIO 13   │ SPI2 (Touch read)   │
 * │ TFT CS       │  GPIO 10   │                     │
 * │ TFT DC       │  GPIO  9   │                     │
 * │ TFT RST      │  GPIO  8   │                     │
 * │ TFT BL (PWM) │  GPIO 38   │ LEDC Ch 1           │
 * │ Touch CS     │  GPIO  7   │ XPT2046             │
 * │ Touch IRQ    │  GPIO  6   │ Optional polling     │
 * ├──────────────┼────────────┼────────────────────┤
 * │ ZIF Pin  1   │  GPIO  1   │                     │
 * │ ZIF Pin  2   │  GPIO  2   │                     │
 * │ ZIF Pin  3   │  GPIO  3   │                     │
 * │ ZIF Pin  4   │  GPIO  5   │ (GPIO4=Battery ADC) │
 * │ ZIF Pin  5   │  GPIO 14   │                     │
 * │ ZIF Pin  6   │  GPIO 15   │                     │
 * │ ZIF Pin  7   │  GPIO 16   │ GND for 14-pin ICs  │
 * │ ZIF Pin  8   │  GPIO 17   │ GND for 16-pin ICs  │
 * │ ZIF Pin  9   │  GPIO 18   │                     │
 * │ ZIF Pin 10   │  GPIO 21   │                     │
 * │ ZIF Pin 11   │  GPIO 39   │                     │
 * │ ZIF Pin 12   │  GPIO 40   │                     │
 * │ ZIF Pin 13   │  GPIO 41   │                     │
 * │ ZIF Pin 14   │  GPIO 42   │ VCC for 14-pin ICs  │
 * │ ZIF Pin 15   │  GPIO 45   │                     │
 * │ ZIF Pin 16   │  GPIO 47   │ VCC for 16-pin ICs  │
 * ├──────────────┼────────────┼────────────────────┤
 * │ Buzzer (PWM) │  GPIO 48   │ LEDC Ch 0, Passive  │
 * │ Battery ADC  │  GPIO  4   │ ADC1_CH3, via divider│
 * └──────────────┴────────────┴────────────────────┘
 *
 * HARDWARE NOTE:
 *   ZIF VCC pins are driven from ESP32 GPIO (3.3V, ~12mA max).
 *   For production, use a P-channel MOSFET to switch the 3.3V
 *   supply rail instead of driving VCC directly from GPIO.
 *
 *   Battery ADC assumes a 100k/100k voltage divider:
 *   VBAT ──[100k]──┬──[100k]── GND
 *                  └── GPIO 4
 *   This halves the voltage: 4.2V → 2.1V, within ADC range.
 */

#pragma once
#include <cstdint>

// ─── Display (defined in platformio.ini build_flags for TFT_eSPI) ─────────
// These are also available as constants for LVGL init
constexpr uint16_t DISPLAY_WIDTH  = 320;  // Logical (landscape after rotation)
constexpr uint16_t DISPLAY_HEIGHT = 240;

// ─── TFT Backlight ────────────────────────────────────────────────────────
constexpr uint8_t  TFT_BL_GPIO    = 38;
constexpr uint8_t  BL_LEDC_CH     = 1;
constexpr uint8_t  BL_LEDC_RES    = 8;    // 8-bit (0-255)
constexpr uint32_t BL_LEDC_FREQ   = 5000; // 5 kHz PWM

// ─── Touch IRQ (optional, for polling we just use TFT.getTouch) ───────────
constexpr uint8_t TOUCH_IRQ_GPIO  = 6;

// ─── ZIF Socket GPIO Map ──────────────────────────────────────────────────
constexpr uint8_t ZIF_GPIO[17] = {
    0,    // index 0 — unused (1-indexed)
    1,    // ZIF Pin  1
    2,    // ZIF Pin  2
    3,    // ZIF Pin  3
    5,    // ZIF Pin  4  (GPIO4 reserved for battery ADC)
    14,   // ZIF Pin  5
    15,   // ZIF Pin  6
    16,   // ZIF Pin  7  (IC GND for 14-pin)
    17,   // ZIF Pin  8  (IC GND for 16-pin)
    18,   // ZIF Pin  9
    21,   // ZIF Pin 10
    39,   // ZIF Pin 11
    40,   // ZIF Pin 12
    41,   // ZIF Pin 13
    42,   // ZIF Pin 14  (IC VCC for 14-pin)
    45,   // ZIF Pin 15
    47,   // ZIF Pin 16  (IC VCC for 16-pin)
};

// ─── Buzzer ───────────────────────────────────────────────────────────────
constexpr uint8_t  BUZZER_GPIO     = 48;
constexpr uint8_t  BUZZER_LEDC_CH  = 0;
constexpr uint8_t  BUZZER_LEDC_RES = 8;

// ─── Battery ADC ──────────────────────────────────────────────────────────
constexpr uint8_t  BATTERY_ADC_PIN  = 4;   // ADC1_CH3
constexpr float    BATTERY_DIVIDER  = 2.0f; // 100k/100k divider ratio
constexpr float    BATT_VMAX        = 4.2f;
constexpr float    BATT_VMIN        = 3.0f;

// ─── Timing ───────────────────────────────────────────────────────────────
constexpr uint32_t SPLASH_DURATION_MS  = 3000;
constexpr uint32_t BATT_POLL_MS        = 10000; // Poll battery every 10s
constexpr uint32_t AUTODETECT_POLL_MS  = 500;   // Check ZIF for IC every 500ms
constexpr uint32_t LVGL_TICK_MS        = 5;     // LVGL task period

// ─── History ──────────────────────────────────────────────────────────────
constexpr uint8_t  MAX_HISTORY_ENTRIES = 20;

// ─── Touch Calibration (typical for 2.8" XPT2046, landscape) ─────────────
// Run tft.calibrateTouch() once to get your exact values
constexpr uint16_t TOUCH_CAL[5] = { 275, 3620, 264, 3532, 1 };
