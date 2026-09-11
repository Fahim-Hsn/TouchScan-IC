/**
 * battery_hal.cpp — Li-Po Battery Monitor Implementation
 *
 * Circuit: VBAT ──[100kΩ]──┬──[100kΩ]── GND
 *                           └── GPIO 4 (ADC1_CH3)
 * This halves VBAT before feeding the ADC (max 3.3V safe for ESP32).
 *
 * Li-Po discharge profile used for % mapping:
 *   4.20V = 100%, 4.00V = 80%, 3.80V = 60%,
 *   3.60V = 40%,  3.40V = 20%, 3.00V = 0%
 */
#include "battery_hal.h"
#include "../config.h"
#include <esp_adc_cal.h>

static esp_adc_cal_characteristics_t adc_chars;

void battery_hal_init(void) {
    // Configure ADC1 — 12-bit, 11dB attenuation (up to ~3.3V input)
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    analogReadResolution(12);

    // Characterise ADC for accurate voltage reading
    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,   // Default Vref (mV) — improve by burning eFuse if needed
        &adc_chars
    );
    Serial.println("[BAT] Battery ADC initialised on GPIO " + String(BATTERY_ADC_PIN));
}

uint32_t battery_hal_get_mv(void) {
    // Oversample 8x to reduce noise
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
        raw += analogRead(BATTERY_ADC_PIN);
    }
    raw /= 8;

    // Convert raw → mV using calibration
    uint32_t adc_mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

    // Apply voltage divider correction (x2 because of 100k/100k divider)
    uint32_t vbat_mv = static_cast<uint32_t>(adc_mv * BATTERY_DIVIDER);
    return vbat_mv;
}

uint8_t battery_hal_get_percent(void) {
    uint32_t mv = battery_hal_get_mv();

    // Clamp to valid range
    if (mv >= static_cast<uint32_t>(BATT_VMAX * 1000)) return 100;
    if (mv <= static_cast<uint32_t>(BATT_VMIN * 1000)) return 0;

    // Li-Po non-linear discharge curve lookup table (mV → %)
    // Pairs: {voltage_mv, percent}
    static const struct { uint32_t mv; uint8_t pct; } curve[] = {
        { 4200, 100 }, { 4150, 97 }, { 4100, 93 }, { 4050, 88 },
        { 4000, 82 }, { 3950, 75 }, { 3900, 68 }, { 3850, 62 },
        { 3800, 55 }, { 3750, 48 }, { 3700, 42 }, { 3650, 35 },
        { 3600, 28 }, { 3550, 22 }, { 3500, 16 }, { 3450, 11 },
        { 3400,  7 }, { 3350,  4 }, { 3300,  2 }, { 3000,   0 },
    };
    constexpr uint8_t TABLE_SIZE = sizeof(curve) / sizeof(curve[0]);

    // Linear interpolation between curve points
    for (uint8_t i = 0; i < TABLE_SIZE - 1; i++) {
        if (mv >= curve[i + 1].mv && mv <= curve[i].mv) {
            uint32_t range_mv  = curve[i].mv - curve[i + 1].mv;
            uint32_t offset_mv = mv - curve[i + 1].mv;
            uint8_t  pct_range = curve[i].pct - curve[i + 1].pct;
            return curve[i + 1].pct + (uint8_t)((offset_mv * pct_range) / range_mv);
        }
    }
    return 0;
}

bool battery_hal_is_critical(void) {
    return battery_hal_get_percent() < 10;
}
