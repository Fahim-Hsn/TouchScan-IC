/**
 * buzzer_hal.h — Passive Buzzer PWM Driver
 * Uses ESP32 LEDC peripheral for tone generation.
 */
#pragma once
#include <Arduino.h>
#include <cstdint>

/** @brief Initialise LEDC channel for the passive buzzer. */
void buzzer_hal_init(void);

/** @brief Enable or disable buzzer globally (respects Settings). */
void buzzer_hal_set_enabled(bool enabled);

/** @brief Play a tone at the given frequency for duration_ms. Blocking. */
void buzzer_hal_tone(uint32_t freq_hz, uint32_t duration_ms);

/** @brief Stop any ongoing tone immediately. */
void buzzer_hal_stop(void);

/**
 * @brief Play "IC GOOD" success melody (non-blocking via LVGL timer).
 *        Two ascending tones: 880Hz → 1320Hz.
 */
void buzzer_hal_beep_good(void);

/**
 * @brief Play "IC FAULTY" error sound (non-blocking via LVGL timer).
 *        Descending buzz: 500Hz → 300Hz.
 */
void buzzer_hal_beep_bad(void);

/**
 * @brief Short confirmation beep when IC is detected (300Hz, 80ms).
 */
void buzzer_hal_beep_detect(void);

/**
 * @brief Soft subtle click confirmation sound on button touch (~2400Hz, 15ms).
 */
void buzzer_hal_beep_click(void);

/**
 * @brief Low-battery warning beep (disabled per user request).
 */
void buzzer_hal_beep_low_battery(void);

