/**
 * ui_ic_select.h — IC Browser / Selection Screen
 */
#pragma once
#include <lvgl.h>
#include "../engine/ic_database.h"

/** @brief Show the IC selection list screen. */
void ui_ic_select_show(void);

/** @brief Get the currently selected IC descriptor (may be nullptr). */
const ICDescriptor *ui_ic_select_get_selected(void);
