/**
 * buzzer_hal.cpp — Active-Low Buzzer (FC-07) Implementation
 *
 * Supports FC-07 Active-Low Trigger (低电平触发) Buzzer module.
 * Logic:
 *   - LOW (0V)  = Buzzer ON (Sound)
 *   - HIGH (3.3V) = Buzzer OFF (Silent)
 */
#include "buzzer_hal.h"
#include "../config.h"

static bool g_buzzer_enabled = true;

void buzzer_hal_init(void) {
    pinMode(BUZZER_GPIO, OUTPUT);
    digitalWrite(BUZZER_GPIO, HIGH); // Silent state for Active-Low trigger
    Serial.println("[BUZ] FC-07 Active-Low buzzer initialised on GPIO " + String(BUZZER_GPIO));
}

void buzzer_hal_set_enabled(bool enabled) {
    g_buzzer_enabled = enabled;
    if (!enabled) {
        digitalWrite(BUZZER_GPIO, HIGH); // Force silent
    }
}

void buzzer_hal_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (!g_buzzer_enabled) return;
    if (duration_ms == 0) duration_ms = 40;

    digitalWrite(BUZZER_GPIO, LOW);  // Turn ON
    delay(duration_ms);
    digitalWrite(BUZZER_GPIO, HIGH); // Turn OFF
}

void buzzer_hal_stop(void) {
    digitalWrite(BUZZER_GPIO, HIGH); // Turn OFF
}

void buzzer_hal_beep_good(void) {
    if (!g_buzzer_enabled) return;
    // 3 short ascending-rhythm beeps
    digitalWrite(BUZZER_GPIO, LOW);
    delay(50);
    digitalWrite(BUZZER_GPIO, HIGH);
    delay(40);
    digitalWrite(BUZZER_GPIO, LOW);
    delay(50);
    digitalWrite(BUZZER_GPIO, HIGH);
    delay(40);
    digitalWrite(BUZZER_GPIO, LOW);
    delay(120);
    digitalWrite(BUZZER_GPIO, HIGH);
}

void buzzer_hal_beep_bad(void) {
    if (!g_buzzer_enabled) return;
    // 2 long alert beeps
    digitalWrite(BUZZER_GPIO, LOW);
    delay(200);
    digitalWrite(BUZZER_GPIO, HIGH);
    delay(80);
    digitalWrite(BUZZER_GPIO, LOW);
    delay(250);
    digitalWrite(BUZZER_GPIO, HIGH);
}

void buzzer_hal_beep_detect(void) {
    if (!g_buzzer_enabled) return;
    // Short crisp chirp
    digitalWrite(BUZZER_GPIO, LOW);
    delay(30);
    digitalWrite(BUZZER_GPIO, HIGH);
}

void buzzer_hal_beep_low_battery(void) {
    if (!g_buzzer_enabled) return;
    digitalWrite(BUZZER_GPIO, LOW);
    delay(120);
    digitalWrite(BUZZER_GPIO, HIGH);
    delay(100);
    digitalWrite(BUZZER_GPIO, LOW);
    delay(120);
    digitalWrite(BUZZER_GPIO, HIGH);
}
