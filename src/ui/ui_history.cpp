/**
 * ui_history.cpp — Test History Screen (Deep Forest Emerald & Mint Theme)
 * Reads history from NVS (Preferences) and shows a scrollable list.
 */
#include "ui_history.h"
#include "ui_settings.h"
#include "ui_home.h"
#include "ui_theme.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <Preferences.h>
#include <stdio.h>

static void on_back(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    ui_settings_show(); // Return to settings
}

static void on_clear_history(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    Preferences prefs;
    prefs.begin("ic_history", false);
    prefs.clear();
    prefs.end();
    ui_history_show();
}

void ui_history_show(void) {
    lv_obj_t *scr = lv_obj_create(nullptr);
    theme_apply_screen(scr);

    // ── TOP HEADER (White Title & Mint Controls) ──────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Back button
    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 64, 28);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 12, 0);
    theme_apply_btn_secondary(btn_back);
    lv_obj_add_event_cb(btn_back, on_back, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_b = lv_label_create(btn_back);
    lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_font(lbl_b, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_b, CLR_TEXT, 0);
    lv_obj_align(lbl_b, LV_ALIGN_CENTER, 0, 0);

    // Title in White
    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, "TEST HISTORY");
    lv_obj_set_style_text_font(lbl_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // Clear button
    lv_obj_t *btn_clr = lv_btn_create(hdr);
    lv_obj_set_size(btn_clr, 56, 28);
    lv_obj_align(btn_clr, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_bg_color(btn_clr, CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(btn_clr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn_clr, CLR_ERROR, 0);
    lv_obj_set_style_border_width(btn_clr, 1, 0);
    lv_obj_set_style_radius(btn_clr, 8, 0);
    lv_obj_set_style_shadow_opa(btn_clr, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(btn_clr, on_clear_history, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, LV_SYMBOL_TRASH " CLR");
    lv_obj_set_style_text_font(lbl_clr, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_clr, CLR_ERROR, 0);
    lv_obj_align(lbl_clr, LV_ALIGN_CENTER, 0, 0);

    // ── Read history from NVS ──────────────────────────────────────────────
    Preferences prefs;
    prefs.begin("ic_history", true);
    uint8_t cnt = prefs.getUChar("cnt", 0);

    // ── Scrollable list (Floating Mint Card) ────────────────────────────────
    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_set_size(list, 296, 188);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 42);
    theme_apply_panel(list);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    if (cnt == 0) {
        lv_obj_t *lbl_empty = lv_label_create(list);
        lv_label_set_text(lbl_empty, "No test history yet.\nTest an IC to see records here.");
        lv_obj_set_style_text_font(lbl_empty, FONT_SMALL, 0);
        lv_obj_set_style_text_color(lbl_empty, CLR_TEXT_DIM, 0);
        lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_empty, LV_ALIGN_CENTER, 0, 0);
    } else {
        for (int8_t i = (int8_t)cnt - 1; i >= 0; i--) {
            char key_ic[12], key_ok[12], key_ms[12];
            snprintf(key_ic, sizeof(key_ic), "ic%d", i);
            snprintf(key_ok, sizeof(key_ok), "ok%d", i);
            snprintf(key_ms, sizeof(key_ms), "ms%d", i);

            char ic_name[8];
            prefs.getString(key_ic, ic_name, sizeof(ic_name));
            bool ok    = prefs.getBool(key_ok, false);
            uint32_t ms = prefs.getULong(key_ms, 0);

            lv_obj_t *row = lv_obj_create(list);
            lv_obj_set_size(row, LV_PCT(100), 38);
            lv_obj_set_style_bg_color(row, ok ? CLR_BLUE_GLOW : CLR_ERROR_DIM, 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(row, ok ? CLR_BLUE_DIM : CLR_ERROR, 0);
            lv_obj_set_style_border_width(row, 1, 0);
            lv_obj_set_style_radius(row, 8, 0);
            lv_obj_set_style_pad_all(row, 4, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            // Entry number (left)
            char num_str[6];
            snprintf(num_str, sizeof(num_str), "#%d", cnt - i);
            lv_obj_t *lbl_num = lv_label_create(row);
            lv_label_set_text(lbl_num, num_str);
            lv_obj_set_style_text_font(lbl_num, FONT_TINY, 0);
            lv_obj_set_style_text_color(lbl_num, CLR_TEXT_DIM, 0);
            lv_obj_align(lbl_num, LV_ALIGN_LEFT_MID, 6, 0);

            // IC name
            lv_obj_t *lbl_ic = lv_label_create(row);
            lv_label_set_text(lbl_ic, ic_name);
            lv_obj_set_style_text_font(lbl_ic, FONT_MEDIUM, 0);
            lv_obj_set_style_text_color(lbl_ic, CLR_TEXT_LABEL, 0);
            lv_obj_align(lbl_ic, LV_ALIGN_LEFT_MID, 36, 0);

            // Result badge
            lv_obj_t *lbl_result = lv_label_create(row);
            lv_label_set_text(lbl_result, ok ? LV_SYMBOL_OK " PASS" : LV_SYMBOL_CLOSE " FAIL");
            lv_obj_set_style_text_font(lbl_result, FONT_SMALL, 0);
            lv_obj_set_style_text_color(lbl_result, ok ? CLR_TEXT : CLR_ERROR, 0);
            lv_obj_align(lbl_result, LV_ALIGN_CENTER, 10, 0);

            // Duration
            char ms_str[12];
            snprintf(ms_str, sizeof(ms_str), "%lums", ms);
            lv_obj_t *lbl_ms = lv_label_create(row);
            lv_label_set_text(lbl_ms, ms_str);
            lv_obj_set_style_text_font(lbl_ms, FONT_TINY, 0);
            lv_obj_set_style_text_color(lbl_ms, CLR_TEXT_DIM, 0);
            lv_obj_align(lbl_ms, LV_ALIGN_RIGHT_MID, -6, 0);
        }
    }
    prefs.end();

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}
