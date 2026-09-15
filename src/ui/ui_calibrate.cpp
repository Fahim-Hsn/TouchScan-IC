/**
 * ui_calibrate.cpp — Interactive 4-point Touch Calibration Screen
 */
#include "ui_calibrate.h"
#include "ui_settings.h"
#include "ui_theme.h"
#include "../hal/tft_hal.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <Preferences.h>
#include <stdio.h>

static lv_obj_t   *scr_cal        = nullptr;
static lv_obj_t   *card_info      = nullptr;
static lv_obj_t   *lbl_step       = nullptr;
static lv_obj_t   *lbl_sub        = nullptr;
static lv_obj_t   *target_dot     = nullptr;
static lv_timer_t *t_poll         = nullptr;

static uint8_t current_step = 0;
static int raw_x[4];
static int raw_y[4];
static bool touch_held = false;

// 4 calibration target points on screen
static const struct {
    lv_coord_t x;
    lv_coord_t y;
    const char *desc;
} cal_points[4] = {
    { 24, 24, "Point 1 / 4: Top-Left" },
    { DISPLAY_WIDTH - 24, 24, "Point 2 / 4: Top-Right" },
    { DISPLAY_WIDTH - 24, DISPLAY_HEIGHT - 24, "Point 3 / 4: Bottom-Right" },
    { 24, DISPLAY_HEIGHT - 24, "Point 4 / 4: Bottom-Left" }
};

static void update_target_position(void) {
    if (current_step >= 4) return;
    
    lv_coord_t tx = cal_points[current_step].x;
    lv_coord_t ty = cal_points[current_step].y;
    
    if (target_dot) {
        lv_obj_set_pos(target_dot, tx - 14, ty - 14);
    }
    
    if (lbl_step) {
        lv_label_set_text(lbl_step, cal_points[current_step].desc);
    }
    if (lbl_sub) {
        lv_label_set_text(lbl_sub, "Tap the red crosshair firmly");
    }
}

static void on_cancel(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    if (t_poll) {
        lv_timer_del(t_poll);
        t_poll = nullptr;
    }
    ui_settings_show();
}

static void save_calibration_and_exit(void) {
    // Extrapolate from the inset margins (24px from edge on 320x240)
    // Left edge (x=0) vs Right edge (x=319)
    // Top edge (y=0) vs Bottom edge (y=239)
    int avg_tl_x = raw_x[0];
    int avg_tl_y = raw_y[0];
    int avg_tr_x = raw_x[1];
    int avg_tr_y = raw_y[1];
    int avg_br_x = raw_x[2];
    int avg_br_y = raw_y[2];
    int avg_bl_x = raw_x[3];
    int avg_bl_y = raw_y[3];

    int cal_x_min = (avg_tl_x + avg_bl_x) / 2;
    int cal_x_max = (avg_tr_x + avg_br_x) / 2;
    int cal_y_min = (avg_tl_y + avg_tr_y) / 2;
    int cal_y_max = (avg_bl_y + avg_br_y) / 2;

    Preferences prefs;
    prefs.begin("touch_cal", false);
    prefs.putInt("x_min", cal_x_min);
    prefs.putInt("x_max", cal_x_max);
    prefs.putInt("y_min", cal_y_min);
    prefs.putInt("y_max", cal_y_max);
    prefs.end();

    tft_hal_set_calibration(cal_x_min, cal_x_max, cal_y_min, cal_y_max);
    buzzer_hal_beep_good();

    if (lbl_step) lv_label_set_text(lbl_step, "✓ CALIBRATION SAVED");
    if (lbl_sub)  lv_label_set_text(lbl_sub, "Returning to settings...");
    if (target_dot) lv_obj_add_flag(target_dot, LV_OBJ_FLAG_HIDDEN);

    // Return to settings after 1 second
    lv_timer_create([](lv_timer_t *t) {
        ui_settings_show();
        lv_timer_del(t);
    }, 1000, nullptr);
}

static void poll_touch_calibration(lv_timer_t *t) {
    int rx = 0, ry = 0;
    bool is_touched = tft_hal_get_raw_touch(&rx, &ry);

    if (is_touched) {
        if (!touch_held && current_step < 4) {
            touch_held = true;
            raw_x[current_step] = rx;
            raw_y[current_step] = ry;
            buzzer_hal_beep_detect();

            current_step++;
            if (current_step < 4) {
                update_target_position();
            } else {
                if (t_poll) {
                    lv_timer_del(t_poll);
                    t_poll = nullptr;
                }
                save_calibration_and_exit();
            }
        }
    } else {
        touch_held = false;
    }
}

void ui_calibrate_show(void) {
    current_step = 0;
    touch_held   = false;

    scr_cal = lv_obj_create(nullptr);
    theme_apply_screen(scr_cal);

    // ── Instruction Center Card (Mint Card) ──────────────────────────────────
    card_info = lv_obj_create(scr_cal);
    lv_obj_set_size(card_info, 220, 130);
    lv_obj_align(card_info, LV_ALIGN_CENTER, 0, 0);
    theme_apply_panel(card_info);
    lv_obj_clear_flag(card_info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(card_info);
    lv_label_set_text(lbl_title, "TOUCH CALIBRATION");
    lv_obj_set_style_text_font(lbl_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_title, CLR_TEXT_LABEL, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);

    lbl_step = lv_label_create(card_info);
    lv_label_set_text(lbl_step, cal_points[0].desc);
    lv_obj_set_style_text_font(lbl_step, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_step, CLR_BG_DARK, 0);
    lv_obj_align(lbl_step, LV_ALIGN_CENTER, 0, -10);

    lbl_sub = lv_label_create(card_info);
    lv_label_set_text(lbl_sub, "Tap the red crosshair firmly");
    lv_obj_set_style_text_font(lbl_sub, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_sub, CLR_TEXT_DIM, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 12);

    // Cancel Button
    lv_obj_t *btn_cancel = lv_btn_create(card_info);
    lv_obj_set_size(btn_cancel, 80, 26);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_MID, 0, -2);
    theme_apply_btn_secondary(btn_cancel);
    lv_obj_add_event_cb(btn_cancel, on_cancel, LV_EVENT_ALL, nullptr);

    lv_obj_t *lbl_c = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_c, "CANCEL");
    lv_obj_set_style_text_font(lbl_c, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_c, CLR_TEXT, 0);
    lv_obj_align(lbl_c, LV_ALIGN_CENTER, 0, 0);

    // ── Target Crosshair Object ──────────────────────────────────────────────
    target_dot = lv_obj_create(scr_cal);
    lv_obj_set_size(target_dot, 28, 28);
    lv_obj_set_pos(target_dot, cal_points[0].x - 14, cal_points[0].y - 14);
    lv_obj_set_style_bg_color(target_dot, CLR_ERROR, 0);
    lv_obj_set_style_border_color(target_dot, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(target_dot, 2, 0);
    lv_obj_set_style_radius(target_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_color(target_dot, CLR_ERROR, 0);
    lv_obj_set_style_shadow_width(target_dot, 10, 0);
    lv_obj_clear_flag(target_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_cross = lv_label_create(target_dot);
    lv_label_set_text(lbl_cross, "+");
    lv_obj_set_style_text_font(lbl_cross, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_cross, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_cross, LV_ALIGN_CENTER, 0, -1);

    lv_scr_load_anim(scr_cal, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);

    // Start raw polling timer (30ms interval)
    t_poll = lv_timer_create(poll_touch_calibration, 30, nullptr);
}
