/**
 * ui_home.cpp — Main Dashboard
 *
 * Layout (320x240 landscape):
 * ┌────────────────────────────────────────┐
 * │ [STATUS BAR] Batt% | Last: Xms         │ h=22
 * ├────────────────────────────────────────┤
 * │                                        │
 * │         INSERT IC  ◉ (pulsing)         │ h=100
 * │      ZIF socket animation              │
 * │                                        │
 * ├────────────────────────────────────────┤
 * │ [TEST IC] [BROWSE IC] [HISTORY] [CFG]  │ h=58
 * └────────────────────────────────────────┘
 */
#include "ui_home.h"
#include "ui_theme.h"
#include "ui_ic_select.h"
#include "ui_history.h"
#include "ui_settings.h"
#include "../config.h"
#include "../hal/battery_hal.h"
#include "../engine/auto_detect.h"
#include "../hal/buzzer_hal.h"
#include <lvgl.h>
#include <stdio.h>

// ─── State ────────────────────────────────────────────────────────────────
static lv_obj_t *scr_home        = nullptr;
static lv_obj_t *lbl_batt        = nullptr;
static lv_obj_t *lbl_lasttest    = nullptr;
static lv_obj_t *lbl_ic_status   = nullptr;
static lv_obj_t *ring_pulse      = nullptr;
static lv_timer_t *t_autodetect  = nullptr;
static lv_timer_t *t_battery     = nullptr;

// ─── Forward declarations ─────────────────────────────────────────────────
static void on_btn_ic_select(lv_event_t *e);
static void on_btn_history(lv_event_t *e);
static void on_btn_settings(lv_event_t *e);
static void auto_detect_poll(lv_timer_t *t);
static void battery_poll(lv_timer_t *t);

// ─── Battery icon helper ──────────────────────────────────────────────────
static void update_batt_label(uint8_t pct) {
    if (!lbl_batt) return;
    char buf[24];
    const char *icon;
    if      (pct > 80) icon = LV_SYMBOL_BATTERY_FULL;
    else if (pct > 60) icon = LV_SYMBOL_BATTERY_3;
    else if (pct > 40) icon = LV_SYMBOL_BATTERY_2;
    else if (pct > 15) icon = LV_SYMBOL_BATTERY_1;
    else               icon = LV_SYMBOL_BATTERY_EMPTY;
    snprintf(buf, sizeof(buf), "%s %d%%", icon, pct);
    lv_label_set_text(lbl_batt, buf);

    if (pct <= 15) lv_obj_set_style_text_color(lbl_batt, CLR_ERROR, 0);
    else if (pct <= 30) lv_obj_set_style_text_color(lbl_batt, CLR_WARNING, 0);
    else lv_obj_set_style_text_color(lbl_batt, CLR_NEON, 0);
}

// ─── Pulse ring animation ─────────────────────────────────────────────────
static void start_pulse_anim(lv_obj_t *obj) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v) {
        lv_obj_set_style_border_opa((lv_obj_t *)o, (lv_opa_t)v, 0);
        lv_coord_t sz = 80 + (lv_coord_t)((255 - v) / 8);
        lv_obj_set_size((lv_obj_t *)o, sz, sz);
        lv_obj_align((lv_obj_t *)o, LV_ALIGN_CENTER, 0, -16);
    });
    lv_anim_set_values(&a, 200, 20);
    lv_anim_set_time(&a, 1200);
    lv_anim_set_playback_time(&a, 1200);
    lv_anim_set_playback_delay(&a, 0);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

// ─── Button callbacks ─────────────────────────────────────────────────────
static void on_btn_ic_select(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
    ui_ic_select_show();
}
static void on_btn_history(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
    ui_history_show();
}
static void on_btn_settings(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
    ui_settings_show();
}

// ─── Auto-detect polling ──────────────────────────────────────────────────
static bool prev_ic_presence = false;
static void auto_detect_poll(lv_timer_t *) {
    // Only poll if still on home screen
    if (lv_scr_act() != scr_home || !lbl_ic_status) return;

    extern bool zif_hal_detect_ic_presence(void);
    bool present = zif_hal_detect_ic_presence();
    if (present != prev_ic_presence) {
        prev_ic_presence = present;
        if (present) {
            lv_label_set_text(lbl_ic_status, LV_SYMBOL_OK " IC DETECTED — Touch to Identify");
            lv_obj_set_style_text_color(lbl_ic_status, CLR_SUCCESS, 0);
            buzzer_hal_beep_detect();
        } else {
            lv_label_set_text(lbl_ic_status, "Insert IC into ZIF Socket");
            lv_obj_set_style_text_color(lbl_ic_status, CLR_TEXT_DIM, 0);
        }
    }
}

// ─── Battery polling ──────────────────────────────────────────────────────
static void battery_poll(lv_timer_t *) {
    uint8_t pct = battery_hal_get_percent();
    update_batt_label(pct);
    if (battery_hal_is_critical()) {
        buzzer_hal_beep_low_battery();
    }
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_home_show(void) {
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }

    scr_home = lv_obj_create(nullptr);
    theme_apply_screen(scr_home);
    
    // Remove the grid lines for a cleaner modern look

    // ── TOP HEADER (Text only) ───────────────────────────────────────────────
    lv_obj_t *header_area = lv_obj_create(scr_home);
    lv_obj_set_size(header_area, DISPLAY_WIDTH, 50);
    lv_obj_align(header_area, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(header_area, LV_OPA_TRANSP, 0); // No background
    lv_obj_set_style_border_width(header_area, 0, 0);
    lv_obj_set_style_pad_all(header_area, 0, 0);
    lv_obj_clear_flag(header_area, LV_OBJ_FLAG_SCROLLABLE);

    // "IC CHECKER" title
    lv_obj_t *lbl_title = lv_label_create(header_area);
    lv_label_set_text(lbl_title, "IC CHECKER");
    lv_obj_set_style_text_font(lbl_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_title, CLR_TEXT_LABEL, 0); // Dark text
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 15, 15);

    // Battery label
    lbl_batt = lv_label_create(header_area);
    lv_obj_set_style_text_font(lbl_batt, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_batt, CLR_TEXT_DIM, 0); // Dim text
    lv_obj_align(lbl_batt, LV_ALIGN_TOP_RIGHT, -15, 15);
    update_batt_label(battery_hal_get_percent());

    // ── CENTRE FLOATING CARD (Light Blue) ────────────────────────────────────
    lv_obj_t *main_card = lv_obj_create(scr_home);
    lv_obj_set_size(main_card, 290, 140);
    lv_obj_align(main_card, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(main_card, CLR_BG_DARK, 0); // Light Blue
    lv_obj_set_style_bg_opa(main_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_card, 16, 0);
    lv_obj_set_style_border_width(main_card, 0, 0);
    
    // Soft shadow for the blue card
    lv_obj_set_style_shadow_color(main_card, CLR_BG_DARK, 0);
    lv_obj_set_style_shadow_width(main_card, 15, 0);
    lv_obj_set_style_shadow_opa(main_card, LV_OPA_30, 0);
    
    lv_obj_clear_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);

    // Pulsing ring inside card
    ring_pulse = lv_obj_create(main_card);
    lv_obj_set_size(ring_pulse, 70, 70);
    lv_obj_align(ring_pulse, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_opa(ring_pulse, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring_pulse, CLR_CYAN, 0);
    lv_obj_set_style_border_width(ring_pulse, 2, 0);
    lv_obj_set_style_radius(ring_pulse, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(ring_pulse, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    start_pulse_anim(ring_pulse);

    // ZIF icon (inner circle) inside card
    lv_obj_t *ic_icon = lv_obj_create(main_card);
    lv_obj_set_size(ic_icon, 46, 46);
    lv_obj_align(ic_icon, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(ic_icon, CLR_BG, 0);
    lv_obj_set_style_border_color(ic_icon, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(ic_icon, 1, 0);
    lv_obj_set_style_radius(ic_icon, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(ic_icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // CPU icon inside
    lv_obj_t *lbl_cpu = lv_label_create(ic_icon);
    lv_label_set_text(lbl_cpu, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(lbl_cpu, FONT_LARGE, 0);
    lv_obj_set_style_text_color(lbl_cpu, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_cpu, LV_ALIGN_TOP_MID, 0, 30);

    // Subtitle
    lv_obj_t *lbl_sub = lv_label_create(main_card);
    lv_label_set_text(lbl_sub, "Insert IC into ZIF socket to begin");
    lv_obj_set_style_text_font(lbl_sub, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(lbl_sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_BOTTOM_MID, 0, -30);

    // IC status label inside card
    lbl_ic_status = lv_label_create(main_card);
    lv_label_set_text(lbl_ic_status, "Insert IC to Begin");
    lv_obj_set_style_text_font(lbl_ic_status, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_ic_status, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_ic_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_ic_status, LV_ALIGN_BOTTOM_MID, 0, -5);

    // ── BOTTOM FLOATING NAVIGATION PILL (Light Green) ──────────────────
    lv_obj_t *nav = lv_obj_create(scr_home);
    lv_obj_set_size(nav, 290, 56);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, -15);
    theme_apply_nav_pill(nav);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Helper lambda to create nav button
    auto make_nav_btn = [&](const char *icon, lv_event_cb_t cb) {
        lv_obj_t *btn = lv_btn_create(nav);
        lv_obj_set_size(btn, 60, 46);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0); // Transparent background for nav buttons
        lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, 0); // No shadow for nav buttons
        lv_obj_set_style_pad_all(btn, 0, 0); // Clear default button padding
        lv_obj_set_style_border_width(btn, 0, 0); // Clear default border
        lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, nullptr);

        lv_obj_t *ico = lv_label_create(btn);
        lv_label_set_text(ico, icon);
        lv_obj_set_style_text_font(ico, FONT_LARGE, 0);
        lv_obj_set_style_text_color(ico, lv_color_hex(0xFFFFFF), 0); // White icon on Green
        lv_obj_align(ico, LV_ALIGN_CENTER, 0, 0);

        return btn;
    };

    make_nav_btn(LV_SYMBOL_LIST, on_btn_ic_select);
    make_nav_btn(LV_SYMBOL_REFRESH, on_btn_history);
    make_nav_btn(LV_SYMBOL_SETTINGS, on_btn_settings);

    // ── Load screen ──────────────────────────────────────────────────
    lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);

    // ── Start background timers ───────────────────────────────────────
    t_autodetect = lv_timer_create(auto_detect_poll, AUTODETECT_POLL_MS, nullptr);
    t_battery    = lv_timer_create(battery_poll, BATT_POLL_MS, nullptr);
}

void ui_home_update_battery(uint8_t pct) {
    update_batt_label(pct);
}

void ui_home_update_last_test_ms(uint32_t ms) {
    if (!lbl_lasttest) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lums", ms);
    lv_label_set_text(lbl_lasttest, buf);
}

void ui_home_ic_detected_flash(void) {
    if (!ring_pulse) return;
    // Brief colour flash to cyan
    lv_obj_set_style_border_color(ring_pulse, CLR_CYAN, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, [](void *, int32_t) {
        lv_obj_set_style_border_color(ring_pulse, CLR_NEON, 0);
    });
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 500);
    lv_anim_start(&a);
}
