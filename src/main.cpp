/**
 * main.cpp — Digital IC Checker Firmware Entry Point
 *
 * ESP32-S3 N16R8 | PlatformIO | Arduino Framework
 * LVGL 8.3 | TFT_eSPI | ST7789 | XPT2046 | Passive Buzzer | Li-Po Battery
 *
 * Boot flow:
 *   lv_init() → HAL init → Splash screen → Home screen
 *   Home: polls ZIF for IC presence (every 500ms)
 *   IC detected → confirm touch → test → result
 *
 * Architecture:
 *   - All LVGL calls on main (Arduino) task/loop
 *   - IC test runs in a FreeRTOS task (ui_test_running.cpp)
 *   - LVGL tick incremented in loop()
 *
 * GPIO pinout: see src/config.h
 */
#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "hal/tft_hal.h"
#include "hal/zif_hal.h"
#include "hal/battery_hal.h"
#include "hal/buzzer_hal.h"
#include "ui/ui_splash.h"

// ─── LVGL tick tracking ───────────────────────────────────────────────────
static uint32_t lv_last_tick = 0;

// ─── Setup ────────────────────────────────────────────────────────────────
void setup(void) {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n============================================");
    Serial.println("   Digital IC Checker | ESP32-S3 N16R8");
    Serial.println("   Firmware v1.0.0");
    Serial.println("============================================\n");

    // 1. LVGL must be initialised first
    lv_init();
    lv_last_tick = millis();

    // 2. Display + touch (registers LVGL display and input drivers)
    tft_hal_init();

    // 3. ZIF socket GPIO (safe high-Z state)
    zif_hal_init();

    // 4. Battery ADC
    battery_hal_init();

    // 5. Passive buzzer
    buzzer_hal_init();

    // 6. Brief startup beep to confirm hardware alive
    buzzer_hal_tone(1000, 50);

    // 7. Show splash screen (transitions to Home automatically)
    ui_splash_show();

    Serial.println("[MAIN] Setup complete. Entering LVGL loop.");
}

// ─── Main loop ────────────────────────────────────────────────────────────
void loop(void) {
    // Increment LVGL tick (must be called regularly)
    uint32_t now = millis();
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;

    // Process LVGL tasks (rendering, animations, timers, events)
    lv_task_handler();

    // Small yield to prevent watchdog timeout and allow FreeRTOS task switching
    delay(LVGL_TICK_MS);
}
