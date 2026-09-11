/**
 * ui_result.h — Test Result Screen
 */
#pragma once
#include <lvgl.h>
#include "../engine/ic_tester.h"

/**
 * @brief Show the detailed result screen after a test completes.
 * @param result  Pointer to completed ICTestResult struct.
 */
void ui_result_show(const ICTestResult *result);
