/**
 * battery_hal.h — Li-Po Battery Monitor
 * Reads battery voltage via ADC (with voltage divider) and returns %.
 */
#pragma once
#include <Arduino.h>
#include <cstdint>

/** @brief Initialise ADC for battery monitoring. */
void battery_hal_init(void);

/**
 * @brief Read battery level.
 * @return 0–100 (%), or 255 if reading failed.
 */
uint8_t battery_hal_get_percent(void);

/**
 * @brief Read battery voltage in millivolts.
 * @return Voltage in mV (e.g. 3700 = 3.7V)
 */
uint32_t battery_hal_get_mv(void);

/**
 * @brief Check if battery is critically low (< 10%).
 */
bool battery_hal_is_critical(void);
