/**
 * ui_settings.cpp — Settings Screen
 * Controls: Buzzer on/off, Brightness slider, Language (future), About
 */
#include "ui_settings.h"
#include "ui_home.h"
#include "ui_theme.h"
#include "../hal/tft_hal.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <Preferences.h>
#include <stdio.h>

static bool buzzer_enabled = true;
static uint8_t brightness  = 90;

static void load_settings(void) {
    Preferences prefs;
    prefs.begin("settings", true);
    buzzer_enabled = prefs.getBool("buzzer", true);
    brightness     = prefs.getUChar("brightness", 90);
    prefs.end();
    // Apply
    buzzer_hal_set_enabled(buzzer_enabled);
    tft_hal_set_brightness(brightness);
}

static void save_settings(void) {
    Preferences prefs;
    prefs.begin("settings", false);
    prefs.putBool("buzzer", buzzer_enabled);
    prefs.putUChar("brightness", brightness);
    prefs.end();
}

static void on_back(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    save_settings();
    ui_home_show();
}

static void on_buzzer_toggle(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *sw = lv_event_get_target(e);
    buzzer_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    buzzer_hal_set_enabled(buzzer_enabled);
    if (buzzer_enabled) buzzer_hal_beep_detect();
}

static void on_brightness_changed(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *slider = lv_event_get_target(e);
    brightness = (uint8_t)lv_slider_get_value(slider);
    tft_hal_set_brightness(brightness);
}

static void on_calibrate_touch(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    save_settings();
    tft_hal_calibrate_touch();
}

void ui_settings_show(void) {
    load_settings();

    lv_obj_t *scr = lv_obj_create(nullptr);
    theme_apply_screen(scr);

    // Grid background removed for cleaner UI

    // ── TOP HEADER (Light Blue Background) ─────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 80);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, CLR_BG_DARK, 0); // Light Blue
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 8, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 55, 22);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
    theme_apply_btn(btn_back);
    lv_obj_add_event_cb(btn_back, on_back, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_b = lv_label_create(btn_back);
    lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_font(lbl_b, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_b, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, "SETTINGS");
    lv_obj_set_style_text_font(lbl_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(lbl_title, 3, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // ── Settings container (Floating White Card) ───────────────────────────
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 90);
    lv_obj_set_pos(cont, 10, 40);
    theme_apply_panel(cont); // White card
    lv_obj_set_style_radius(cont, 16, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 4, 0);
    lv_obj_set_style_pad_row(cont, 10, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

    // Helper to make a setting row
    auto make_row = [&](const char *icon, const char *label_text) -> lv_obj_t * {
        lv_obj_t *row = lv_obj_create(cont);
        lv_obj_set_size(row, LV_PCT(100), 40);
        theme_apply_panel(row);
        lv_obj_set_style_pad_all(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_icon = lv_label_create(row);
        lv_label_set_text(lbl_icon, icon);
        lv_obj_set_style_text_color(lbl_icon, CLR_NEON, 0);
        lv_obj_set_style_text_font(lbl_icon, FONT_NORMAL, 0);
        lv_obj_align(lbl_icon, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, label_text);
        lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
        lv_obj_set_style_text_color(lbl, CLR_TEXT, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 28, 0);
        return row;
    };

    // ── Row 1: Buzzer ──────────────────────────────────────────────────────
    lv_obj_t *row_buz = make_row(LV_SYMBOL_AUDIO, "Buzzer");
    lv_obj_t *sw_buz  = lv_switch_create(row_buz);
    lv_obj_align(sw_buz, LV_ALIGN_RIGHT_MID, 0, 0);
    if (buzzer_enabled) lv_obj_add_state(sw_buz, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw_buz, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw_buz, CLR_NEON, LV_PART_KNOB);
    lv_obj_add_event_cb(sw_buz, on_buzzer_toggle, LV_EVENT_ALL, nullptr);

    // ── Row 2: Brightness ─────────────────────────────────────────────────
    lv_obj_t *row_bri = make_row(LV_SYMBOL_IMAGE, "Brightness");
    lv_obj_t *slider  = lv_slider_create(row_bri);
    lv_obj_set_size(slider, 110, 10);
    lv_obj_align(slider, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_slider_set_range(slider, 20, 100);
    lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, CLR_CYAN, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, CLR_BG_PANEL, LV_PART_MAIN);
    lv_obj_add_event_cb(slider, on_brightness_changed, LV_EVENT_ALL, nullptr);

    // ── Row 3: Touch Calibration ──────────────────────────────────────────
    lv_obj_t *row_cal = make_row(LV_SYMBOL_SETTINGS, "Touch Calibrate");
    lv_obj_t *btn_cal = lv_btn_create(row_cal);
    lv_obj_set_size(btn_cal, 70, 26);
    lv_obj_align(btn_cal, LV_ALIGN_RIGHT_MID, 0, 0);
    theme_apply_btn(btn_cal);
    lv_obj_add_event_cb(btn_cal, on_calibrate_touch, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_cal = lv_label_create(btn_cal);
    lv_label_set_text(lbl_cal, "CALIBRATE");
    lv_obj_set_style_text_font(lbl_cal, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_cal, CLR_NEON, 0);
    lv_obj_align(lbl_cal, LV_ALIGN_CENTER, 0, 0);

    // ── Row 4: Language (v1 skeleton) ─────────────────────────────────────
    lv_obj_t *row_lang = make_row(LV_SYMBOL_LIST, "Language");
    lv_obj_t *lbl_lang_val = lv_label_create(row_lang);
    lv_label_set_text(lbl_lang_val, "English  (বাংলা soon)");
    lv_obj_set_style_text_font(lbl_lang_val, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_lang_val, CLR_TEXT_LABEL, 0);
    lv_obj_align(lbl_lang_val, LV_ALIGN_RIGHT_MID, -4, 0);

    // ── About row ─────────────────────────────────────────────────────────
    lv_obj_t *lbl_about = lv_label_create(cont);
    lv_label_set_text(lbl_about,
        "IC Checker v1.0  |  ESP32-S3 N16R8\n"
        "GPIO mapping: see config.h");
    lv_obj_set_style_text_font(lbl_about, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_about, CLR_TEXT_LABEL, 0);
    lv_obj_set_style_text_align(lbl_about, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_about, DISPLAY_WIDTH - 24);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}
