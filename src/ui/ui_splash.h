/**
 * ui_splash.h — Animated Sci-Fi Splash / Boot Screen
 */
#pragma once
#include <lvgl.h>

/**
 * @brief Show the splash screen and begin boot animation.
 *        Automatically transitions to Home screen after SPLASH_DURATION_MS.
 */
void ui_splash_show(void);

/**
 * @brief Destroy the splash screen and free its resources.
 *        Called internally after transition, exposed for testing.
 */
void ui_splash_destroy(void);
