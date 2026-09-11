/**
 * ui_test_running.h — Test In Progress Screen
 */
#pragma once
#include <lvgl.h>
#include "../engine/ic_database.h"

/**
 * @brief Show the animated testing screen and begin the IC test.
 * @param ic          IC to test (from database)
 * @param auto_detect true if IC was auto-detected (shows confidence)
 */
void ui_test_running_show(const ICDescriptor *ic, bool auto_detect);
