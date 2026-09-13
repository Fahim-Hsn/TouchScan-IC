/**
 * ui_home.cpp — Main Dashboard (Deep Forest Emerald & Mint Theme)
 *
 * Layout (320x240 landscape):
 * ┌────────────────────────────────────────┐
 * │ ⚡ IC CHECKER               [ 🔋 100% ] │ h=38 (White text on Deep Emerald)
 * ├────────────────────────────────────────┤
 * │ ┌────────────────────────────────────┐ │
 * │ │        [ READY TO TEST ]           │ │
 * │ │      ◉ ZIF Socket Animation        │ │ h=124 (Solid Mint Floating Card)
 * │ │        "Insert IC to begin"        │ │
 * │ │        [ ▶ START TEST ]            │ │
 * │ └────────────────────────────────────┘ │
 * ├────────────────────────────────────────┤
 * │  [ 🔍 SELECT ] [ ⚡ AUTO ] [ ⚙ CONFIG ] │ h=52 (Mint Floating Nav Pill)
 * └────────────────────────────────────────┘
 */
#include "ui_home.h"
#include "ui_theme.h"
#include "ui_ic_select.h"
#include "ui_history.h"
#include "ui_settings.h"
#include "ui_test_running.h"
#include "../config.h"
#include "../hal/battery_hal.h"
#include "../hal/zif_hal.h"
#include "../engine/auto_detect.h"
#include "../hal/buzzer_hal.h"
#include <lvgl.h>
#include <stdio.h>

// ─── State ────────────────────────────────────────────────────────────────
static lv_obj_t *scr_home        = nullptr;
static lv_obj_t *lbl_batt        = nullptr;
static lv_obj_t *lbl_lasttest    = nullptr;
static lv_obj_t *lbl_ic_status   = nullptr;
static lv_obj_t *badge_status    = nullptr;
static lv_obj_t *lbl_badge_txt   = nullptr;
static lv_obj_t *ring_pulse      = nullptr;
static lv_timer_t *t_autodetect  = nullptr;
static lv_timer_t *t_battery     = nullptr;
static lv_obj_t *warn_modal      = nullptr;

// ─── Forward declarations ─────────────────────────────────────────────────
static void on_btn_ic_select(lv_event_t *e);
static void on_btn_auto_test(lv_event_t *e);
static void on_btn_settings(lv_event_t *e);
static void on_btn_start_test(lv_event_t *e);
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

    if (pct <= 15) {
        lv_obj_set_style_text_color(lbl_batt, CLR_ERROR, 0);
    } else if (pct <= 30) {
        lv_obj_set_style_text_color(lbl_batt, CLR_WARNING, 0);
    } else {
        lv_obj_set_style_text_color(lbl_batt, CLR_TEXT, 0);
    }
}

// ─── Pulse ring animation ─────────────────────────────────────────────────
static void start_pulse_anim(lv_obj_t *obj) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v) {
        lv_obj_set_style_border_opa((lv_obj_t *)o, (lv_opa_t)v, 0);
        lv_coord_t sz = 54 + (lv_coord_t)((255 - v) / 10);
        lv_obj_set_size((lv_obj_t *)o, sz, sz);
        lv_obj_align((lv_obj_t *)o, LV_ALIGN_CENTER, 0, -8);
    });
    lv_anim_set_values(&a, 200, 20);
    lv_anim_set_time(&a, 1200);
    lv_anim_set_playback_time(&a, 1200);
    lv_anim_set_playback_delay(&a, 0);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

// ─── Modal Popup helper ───────────────────────────────────────────────────
static void close_modal(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (warn_modal) {
        lv_obj_del(warn_modal);
        warn_modal = nullptr;
    }
}

static void show_modal(const char *title, const char *msg, bool is_error) {
    if (warn_modal) return;
    
    warn_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(warn_modal, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(warn_modal, 0, 0);
    lv_obj_set_style_bg_color(warn_modal, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_bg_opa(warn_modal, LV_OPA_70, 0);
    lv_obj_set_style_border_width(warn_modal, 0, 0);
    lv_obj_clear_flag(warn_modal, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *card = lv_obj_create(warn_modal);
    lv_obj_set_size(card, 264, 130);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    theme_apply_panel(card);
    lv_obj_set_style_border_color(card, is_error ? CLR_ERROR : CLR_WARNING, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl_t = lv_label_create(card);
    lv_label_set_text(lbl_t, title);
    lv_obj_set_style_text_font(lbl_t, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_t, is_error ? CLR_ERROR : CLR_WARNING, 0);
    lv_obj_align(lbl_t, LV_ALIGN_TOP_MID, 0, 4);
    
    lv_obj_t *lbl_m = lv_label_create(card);
    lv_label_set_text(lbl_m, msg);
    lv_obj_set_style_text_font(lbl_m, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_m, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_m, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_m, LV_ALIGN_CENTER, 0, 4);
    
    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_size(btn_ok, 90, 30);
    lv_obj_align(btn_ok, LV_ALIGN_BOTTOM_MID, 0, -4);
    theme_apply_btn(btn_ok);
    lv_obj_add_event_cb(btn_ok, close_modal, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "OK");
    lv_obj_set_style_text_font(lbl_ok, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_ok, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_ok, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_event_cb(warn_modal, close_modal, LV_EVENT_ALL, nullptr);
}

// ─── Button callbacks ─────────────────────────────────────────────────────
static void on_btn_ic_select(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
    ui_ic_select_show();
}

static void on_btn_auto_test(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    if (!zif_hal_detect_ic_presence()) {
        buzzer_hal_beep_bad();
        show_modal(LV_SYMBOL_WARNING "  NO IC DETECTED", "Please insert IC into ZIF socket\nbefore auto-testing.", true);
        return;
    }
    
    // Attempt auto-detection
    uint8_t confidence = 0;
    const ICDescriptor *detected = auto_detect_ic(&confidence);
    
    if (detected) {
        buzzer_hal_beep_detect();
        if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
        if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
        ui_test_running_show(detected, true);
    } else {
        buzzer_hal_beep_bad();
        show_modal(LV_SYMBOL_WARNING "  UNKNOWN IC", "Could not identify IC pinout.\nTry selecting manually from list.", false);
    }
}

static void on_btn_settings(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
    if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
    ui_settings_show();
}

static void on_btn_start_test(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    const ICDescriptor *sel = ui_ic_select_get_selected();
    if (sel && zif_hal_detect_ic_presence()) {
        if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
        if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
        ui_test_running_show(sel, false);
    } else {
        if (t_autodetect) { lv_timer_del(t_autodetect); t_autodetect = nullptr; }
        if (t_battery)    { lv_timer_del(t_battery);    t_battery    = nullptr; }
        ui_ic_select_show();
    }
}

// ─── Auto-detect polling ──────────────────────────────────────────────────
static bool prev_ic_presence = false;
static void auto_detect_poll(lv_timer_t *) {
    if (lv_scr_act() != scr_home || !lbl_ic_status) return;

    bool present = zif_hal_detect_ic_presence();
    if (present != prev_ic_presence) {
        prev_ic_presence = present;
        if (present) {
            lv_label_set_text(lbl_ic_status, LV_SYMBOL_OK " IC Detected in ZIF");
            lv_obj_set_style_text_color(lbl_ic_status, CLR_BG_DARK, 0);
            
            if (lbl_badge_txt) lv_label_set_text(lbl_badge_txt, "IC READY");
            if (badge_status) {
                lv_obj_set_style_bg_color(badge_status, CLR_SUCCESS_DIM, 0);
                lv_obj_set_style_border_color(badge_status, CLR_SUCCESS, 0);
            }
            buzzer_hal_beep_detect();
        } else {
            lv_label_set_text(lbl_ic_status, "Insert IC into ZIF Socket");
            lv_obj_set_style_text_color(lbl_ic_status, CLR_TEXT_DIM, 0);
            
            if (lbl_badge_txt) lv_label_set_text(lbl_badge_txt, "STANDBY");
            if (badge_status) {
                lv_obj_set_style_bg_color(badge_status, CLR_BLUE_DIM, 0);
                lv_obj_set_style_border_color(badge_status, CLR_SEPARATOR, 0);
            }
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

    // ── TOP HEADER (White Text against Deep Forest Emerald Canvas) ────────────
    lv_obj_t *header_area = lv_obj_create(scr_home);
    lv_obj_set_size(header_area, DISPLAY_WIDTH, 38);
    lv_obj_align(header_area, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(header_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header_area, 0, 0);
    lv_obj_set_style_pad_all(header_area, 0, 0);
    lv_obj_clear_flag(header_area, LV_OBJ_FLAG_SCROLLABLE);

    // "IC CHECKER" Brand Title in White
    lv_obj_t *lbl_title = lv_label_create(header_area);
    lv_label_set_text(lbl_title, LV_SYMBOL_CHARGE " IC CHECKER");
    lv_obj_set_style_text_font(lbl_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 14, 0);

    // Battery pill badge in Mint (Right)
    lv_obj_t *batt_pill = lv_obj_create(header_area);
    lv_obj_set_size(batt_pill, 80, 26);
    lv_obj_align(batt_pill, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_bg_color(batt_pill, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(batt_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(batt_pill, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(batt_pill, 1, 0);
    lv_obj_set_style_radius(batt_pill, 13, 0);
    lv_obj_set_style_pad_all(batt_pill, 0, 0);
    lv_obj_clear_flag(batt_pill, LV_OBJ_FLAG_SCROLLABLE);

    lbl_batt = lv_label_create(batt_pill);
    lv_obj_set_style_text_font(lbl_batt, FONT_TINY, 0);
    lv_obj_align(lbl_batt, LV_ALIGN_CENTER, 0, 0);
    update_batt_label(battery_hal_get_percent());

    // ── CENTRE HERO CARD (Solid Mint Card) ───────────────────────────────────
    lv_obj_t *hero_card = lv_obj_create(scr_home);
    lv_obj_set_size(hero_card, 296, 126);
    lv_obj_align(hero_card, LV_ALIGN_TOP_MID, 0, 42);
    theme_apply_panel(hero_card);
    lv_obj_clear_flag(hero_card, LV_OBJ_FLAG_SCROLLABLE);

    // Status Badge inside Mint card
    badge_status = lv_obj_create(hero_card);
    lv_obj_set_size(badge_status, 110, 22);
    lv_obj_align(badge_status, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(badge_status, CLR_BLUE_GLOW, 0);
    lv_obj_set_style_bg_opa(badge_status, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge_status, CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(badge_status, 1, 0);
    lv_obj_set_style_radius(badge_status, 11, 0);
    lv_obj_set_style_pad_all(badge_status, 0, 0);
    lv_obj_clear_flag(badge_status, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_txt = lv_label_create(badge_status);
    lv_label_set_text(lbl_badge_txt, "READY TO TEST");
    lv_obj_set_style_text_font(lbl_badge_txt, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_badge_txt, CLR_TEXT, 0);
    lv_obj_align(lbl_badge_txt, LV_ALIGN_CENTER, 0, 0);

    // Pulsing outer ring
    ring_pulse = lv_obj_create(hero_card);
    lv_obj_set_size(ring_pulse, 54, 54);
    lv_obj_align(ring_pulse, LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_style_bg_opa(ring_pulse, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring_pulse, CLR_NEON, 0);
    lv_obj_set_style_border_width(ring_pulse, 2, 0);
    lv_obj_set_style_radius(ring_pulse, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(ring_pulse, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    start_pulse_anim(ring_pulse);

    // ZIF socket icon (inner circle)
    lv_obj_t *ic_icon = lv_obj_create(hero_card);
    lv_obj_set_size(ic_icon, 38, 38);
    lv_obj_align(ic_icon, LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_style_bg_color(ic_icon, CLR_BLUE_GLOW, 0);
    lv_obj_set_style_border_color(ic_icon, CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(ic_icon, 1, 0);
    lv_obj_set_style_radius(ic_icon, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(ic_icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_cpu = lv_label_create(ic_icon);
    lv_label_set_text(lbl_cpu, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(lbl_cpu, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_cpu, CLR_BG_DARK, 0);
    lv_obj_align(lbl_cpu, LV_ALIGN_CENTER, 0, 0);

    // IC status sub-label
    lbl_ic_status = lv_label_create(hero_card);
    lv_label_set_text(lbl_ic_status, "Insert IC into ZIF Socket");
    lv_obj_set_style_text_font(lbl_ic_status, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_ic_status, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_ic_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_ic_status, LV_ALIGN_BOTTOM_MID, 0, -4);

    // ── BOTTOM FLOATING NAVIGATION PILL (SELECT | AUTO | CONFIG) ─────────────
    lv_obj_t *nav = lv_obj_create(scr_home);
    lv_obj_set_size(nav, 296, 54);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, -10);
    theme_apply_nav_pill(nav);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Helper to create clean nav tab button with icon + short text
    auto make_nav_tab = [&](const char *icon, const char *label_text, lv_event_cb_t cb, bool is_primary) {
        lv_obj_t *btn = lv_btn_create(nav);
        lv_obj_set_size(btn, 86, 42);
        lv_obj_set_style_bg_color(btn, is_primary ? CLR_BG_DARK : CLR_BLUE_GLOW, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, is_primary ? CLR_NEON : CLR_BLUE_DIM, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_pad_all(btn, 2, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, 0);

        // Pressed state
        lv_obj_set_style_bg_color(btn, is_primary ? CLR_CYAN : CLR_BG_DARK, LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, nullptr);

        lv_obj_t *cont_btn = lv_obj_create(btn);
        lv_obj_set_size(cont_btn, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(cont_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont_btn, 0, 0);
        lv_obj_set_style_pad_all(cont_btn, 0, 0);
        lv_obj_clear_flag(cont_btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ico = lv_label_create(cont_btn);
        lv_label_set_text(ico, icon);
        lv_obj_set_style_text_font(ico, FONT_SMALL, 0);
        lv_obj_set_style_text_color(ico, is_primary ? lv_color_hex(0xFFFFFF) : CLR_BG_DARK, 0);
        lv_obj_align(ico, LV_ALIGN_TOP_MID, 0, 2);

        lv_obj_t *txt = lv_label_create(cont_btn);
        lv_label_set_text(txt, label_text);
        lv_obj_set_style_text_font(txt, FONT_TINY, 0);
        lv_obj_set_style_text_color(txt, is_primary ? lv_color_hex(0xFFFFFF) : CLR_TEXT, 0);
        lv_obj_align(txt, LV_ALIGN_BOTTOM_MID, 0, -2);

        return btn;
    };

    make_nav_tab(LV_SYMBOL_LIST, "SELECT", on_btn_ic_select, false);
    make_nav_tab(LV_SYMBOL_PLAY, "AUTO", on_btn_auto_test, true);    // Highlighted AUTO test button
    make_nav_tab(LV_SYMBOL_SETTINGS, "CONFIG", on_btn_settings, false);

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
    lv_obj_set_style_border_color(ring_pulse, CLR_SUCCESS, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, [](void *, int32_t) {
        lv_obj_set_style_border_color(ring_pulse, CLR_NEON, 0);
    });
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 500);
    lv_anim_start(&a);
}
