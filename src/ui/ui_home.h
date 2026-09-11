/**
 * ui_home.h — Main Dashboard / Home Screen
 */
#pragma once
#include <lvgl.h>

/** @brief Show the Home screen (creates and loads a new lv_obj screen). */
void ui_home_show(void);

/** @brief Update the battery percentage display on the status bar. */
void ui_home_update_battery(uint8_t pct);

/** @brief Update the last test time on the status bar. */
void ui_home_update_last_test_ms(uint32_t ms);

/** @brief Flash the IC detection indicator (called when IC is placed). */
void ui_home_ic_detected_flash(void);
