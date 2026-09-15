/**
 * buzzer_hal.cpp — PWM Tone Driver for FC-07 Buzzer Module
 *
 * Supports both Active and Passive buzzers on FC-07 (Low-Level Trigger) modules:
 * - Emits hardware LEDC square wave (PWM) at resonant frequencies (2kHz - 3.5kHz)
 *   so passive diaphragms vibrate loudly and cleanly.
 * - When silent, forces GPIO to HIGH (3.3V) via ledcDetachPin to prevent DC current draw
 *   and keep the PNP transistor completely OFF.
 */
#include "buzzer_hal.h"
#include "../config.h"

static bool g_buzzer_enabled = true;

static void set_silent(void) {
    ledcWriteTone(BUZZER_LEDC_CH, 0);
    ledcDetachPin(BUZZER_GPIO);
    pinMode(BUZZER_GPIO, OUTPUT);
    digitalWrite(BUZZER_GPIO, HIGH); // Silent state for Active-Low module (PNP cutoff)
}

void buzzer_hal_init(void) {
    pinMode(BUZZER_GPIO, OUTPUT);
    digitalWrite(BUZZER_GPIO, HIGH); // Silent state on boot
    ledcSetup(BUZZER_LEDC_CH, 2700, BUZZER_LEDC_RES);
    Serial.println("[BUZ] FC-07 PWM Tone Buzzer initialised on GPIO " + String(BUZZER_GPIO));
}

void buzzer_hal_set_enabled(bool enabled) {
    g_buzzer_enabled = enabled;
    if (!enabled) {
        set_silent();
    }
}

void buzzer_hal_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (!g_buzzer_enabled || freq_hz == 0) return;

    ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC_CH);
    ledcWriteTone(BUZZER_LEDC_CH, freq_hz);
    if (duration_ms > 0) {
        delay(duration_ms);
        set_silent();
    }
}

void buzzer_hal_stop(void) {
    set_silent();
}

void buzzer_hal_beep_good(void) {
    if (!g_buzzer_enabled) return;
    // Crisp 3-tone ascending success arpeggio
    buzzer_hal_tone(2000, 50);
    delay(25);
    buzzer_hal_tone(2600, 50);
    delay(25);
    buzzer_hal_tone(3300, 120);
}

void buzzer_hal_beep_bad(void) {
    if (!g_buzzer_enabled) return;
    // Distinct 2-tone error alert
    buzzer_hal_tone(900, 140);
    delay(40);
    buzzer_hal_tone(600, 220);
}

void buzzer_hal_beep_detect(void) {
    if (!g_buzzer_enabled) return;
    buzzer_hal_tone(2800, 40);
}

void buzzer_hal_beep_click(void) {
    if (!g_buzzer_enabled) return;
    
    // Cool, smooth futuristic micro-pip with low duty cycle for soft loudness
    ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC_CH);
    
    // First micro-tone: 3400Hz soft pulse (7ms)
    ledcSetup(BUZZER_LEDC_CH, 3400, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CH, 232); // Low duty cycle for quiet, gentle volume
    delay(7);
    
    // Second micro-tone: 4600Hz crystal blip (9ms)
    ledcSetup(BUZZER_LEDC_CH, 4600, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CH, 232);
    delay(9);
    
    set_silent();
}

void buzzer_hal_beep_low_battery(void) {
    // Disabled per user request: no sound for low charge
}



