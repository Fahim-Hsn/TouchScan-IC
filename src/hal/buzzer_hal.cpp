/**
 * buzzer_hal.cpp — Passive Buzzer PWM Implementation
 */
#include "buzzer_hal.h"
#include "../config.h"

static bool g_buzzer_enabled = true;

void buzzer_hal_init(void) {
    ledcSetup(BUZZER_LEDC_CH, 1000, BUZZER_LEDC_RES);
    ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC_CH);
    ledcWrite(BUZZER_LEDC_CH, 0); // Silent
    Serial.println("[BUZ] Passive buzzer initialised on GPIO " + String(BUZZER_GPIO));
}

void buzzer_hal_set_enabled(bool enabled) {
    g_buzzer_enabled = enabled;
    if (!enabled) ledcWrite(BUZZER_LEDC_CH, 0);
}

void buzzer_hal_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (!g_buzzer_enabled) return;
    if (freq_hz == 0) {
        ledcWrite(BUZZER_LEDC_CH, 0);
    } else {
        ledcWriteTone(BUZZER_LEDC_CH, freq_hz);
        ledcWrite(BUZZER_LEDC_CH, 128); // 50% duty = loudest on passive buzzer
    }
    if (duration_ms > 0) {
        delay(duration_ms);
        ledcWrite(BUZZER_LEDC_CH, 0);
    }
}

void buzzer_hal_stop(void) {
    ledcWrite(BUZZER_LEDC_CH, 0);
}

void buzzer_hal_beep_good(void) {
    if (!g_buzzer_enabled) return;
    // Ascending: 880 Hz → 1046 Hz → 1320 Hz
    buzzer_hal_tone(880,  80);
    delay(20);
    buzzer_hal_tone(1046, 80);
    delay(20);
    buzzer_hal_tone(1320, 150);
}

void buzzer_hal_beep_bad(void) {
    if (!g_buzzer_enabled) return;
    // Descending: 500 Hz → 350 Hz → 250 Hz (harsh)
    buzzer_hal_tone(500,  100);
    delay(30);
    buzzer_hal_tone(350,  100);
    delay(30);
    buzzer_hal_tone(250,  200);
}

void buzzer_hal_beep_detect(void) {
    if (!g_buzzer_enabled) return;
    // Short friendly chirp
    buzzer_hal_tone(1000, 40);
    delay(15);
    buzzer_hal_tone(1200, 60);
}

void buzzer_hal_beep_low_battery(void) {
    if (!g_buzzer_enabled) return;
    // Two short low-pitched beeps
    buzzer_hal_tone(400, 100);
    delay(100);
    buzzer_hal_tone(400, 100);
}
