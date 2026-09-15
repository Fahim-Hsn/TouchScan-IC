/**
 * ui_result.cpp — Detailed IC Test Result Screen (Deep Forest Emerald & Mint Theme)
 *
 * Layout (320x240 landscape):
 * ┌──────────────────────────────────────────┐
 * │ [ ✓ PASSED ]                 7408 | 32ms │  Header banner (White text / Mint pill)
 * ├──────────────────────────────────────────┤
 * │ ┌──────────────────────────────────────┐ │
 * │ │  1 · [   74HC08   ] · 14             │ │
 * │ │  2 · [ G1:✓  G2:✗ ] · 13             │ │  Faulty gate pin LEDs blink RED
 * │ │  3 · [ G3:✓  G4:✓ ] · 12             │ │
 * │ │     [ ⚠ GOOD: 3 | FAULTY: 1 ]        │ │
 * │ └──────────────────────────────────────┘ │
 * ├──────────────────────────────────────────┤
 * │  [ 🏠 HOME ]  [ ↺ RETEST ]  [ 💾 SAVE ]  │  Bottom Action Buttons (h=36)
 * └──────────────────────────────────────────┘
 */
#include "ui_result.h"
#include "ui_home.h"
#include "ui_history.h"
#include "ui_test_running.h"
#include "ui_theme.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t       *scr_result   = nullptr;
static const ICTestResult *g_result  = nullptr;

// ─── History save ─────────────────────────────────────────────────────────
static void save_to_history(const ICTestResult *r) {
    Preferences prefs;
    prefs.begin("ic_history", false);

    uint8_t cnt = prefs.getUChar("cnt", 0);
    if (cnt >= MAX_HISTORY_ENTRIES) {
        for (uint8_t i = 0; i < MAX_HISTORY_ENTRIES - 1; i++) {
            char src[12], dst[12];
            snprintf(src, sizeof(src), "ic%d", i + 1);
            snprintf(dst, sizeof(dst), "ic%d", i);
            char val[8];
            prefs.getString(src, val, sizeof(val));
            prefs.putString(dst, val);

            snprintf(src, sizeof(src), "ok%d", i + 1);
            snprintf(dst, sizeof(dst), "ok%d", i);
            prefs.putBool(dst, prefs.getBool(src, false));

            snprintf(src, sizeof(src), "ms%d", i + 1);
            snprintf(dst, sizeof(dst), "ms%d", i);
            prefs.putULong(dst, prefs.getULong(src, 0));
        }
        cnt = MAX_HISTORY_ENTRIES - 1;
    }

    char key_ic[12], key_ok[12], key_ms[12];
    snprintf(key_ic, sizeof(key_ic), "ic%d", cnt);
    snprintf(key_ok, sizeof(key_ok), "ok%d", cnt);
    snprintf(key_ms, sizeof(key_ms), "ms%d", cnt);

    prefs.putString(key_ic, r->ic->ic_number);
    prefs.putBool(key_ok, r->overall_pass);
    prefs.putULong(key_ms, r->test_duration_ms);
    prefs.putUChar("cnt", cnt + 1);
    prefs.end();
}

// ─── IC Visualization ─────────────────────────────────────────────────────
static void draw_ic_visualization(lv_obj_t *parent, const ICTestResult *r) {
    const ICDescriptor *ic = r->ic;
    
    // Floating Mint Card (h=142)
    lv_obj_t *ic_area = lv_obj_create(parent);
    lv_obj_set_size(ic_area, 296, 142);
    lv_obj_align(ic_area, LV_ALIGN_TOP_MID, 0, 42);
    theme_apply_panel(ic_area);
    lv_obj_clear_flag(ic_area, LV_OBJ_FLAG_SCROLLABLE);

    // Physical IC Body (86x86px, compact, clean)
    lv_obj_t *body = lv_obj_create(ic_area);
    lv_obj_set_size(body, 86, 86);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(body, r->overall_pass ? CLR_NEON : CLR_ERROR, 0);
    lv_obj_set_style_border_width(body, 1, 0);
    lv_obj_set_style_radius(body, 6, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // IC Notch (Top center)
    lv_obj_t *notch = lv_obj_create(body);
    lv_obj_set_size(notch, 14, 14);
    lv_obj_align(notch, LV_ALIGN_TOP_MID, 0, -9);
    lv_obj_set_style_radius(notch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(notch, CLR_BG_PANEL, 0);
    lv_obj_set_style_border_color(notch, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(notch, 1, 0);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_SCROLLABLE);
    
    // IC Label text inside body
    lv_obj_t *lbl_ic = lv_label_create(body);
    
    char body_txt[128];
    int offset = snprintf(body_txt, sizeof(body_txt), "%s\n", ic->ic_number);
    
    for (uint8_t g = 0; g < ic->num_gates; g++) {
        offset += snprintf(body_txt + offset, sizeof(body_txt) - offset, 
            "G%d:%s ", g + 1, r->gate_results[g].gate_pass ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
        if (g % 2 == 1 && g < ic->num_gates - 1) { 
            offset += snprintf(body_txt + offset, sizeof(body_txt) - offset, "\n");
        }
    }

    lv_label_set_text(lbl_ic, body_txt);
    lv_obj_set_style_text_color(lbl_ic, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_ic, FONT_TINY, 0);
    lv_obj_set_style_text_align(lbl_ic, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_ic, 80);
    lv_obj_align(lbl_ic, LV_ALIGN_CENTER, 0, 2);

    // Count good vs bad gates
    uint8_t good_gates = 0;
    uint8_t bad_gates = 0;
    for (uint8_t g = 0; g < ic->num_gates; g++) {
        if (r->gate_results[g].gate_pass) good_gates++;
        else bad_gates++;
    }

    // Draw Pins aligned cleanly within 86px height
    uint8_t pins_per_side = ic->pin_count / 2;
    uint16_t pin_spacing = 84 / pins_per_side;
    
    for (uint8_t i = 0; i < ic->pin_count; i++) {
        bool is_left = (i < pins_per_side);
        uint8_t row_idx = is_left ? i : (ic->pin_count - 1 - i);
        uint8_t zif_pin = i + 1;
        
        lv_coord_t py = 4 + row_idx * pin_spacing + (pin_spacing / 2) - 3;
        
        lv_color_t dot_color = CLR_TEXT_DIM;
        bool is_faulty_pin = false;
        
        for (uint8_t g = 0; g < ic->num_gates; g++) {
            if (ic->gates[g].output_pin == zif_pin) {
                if (r->gate_results[g].gate_pass) {
                    dot_color = CLR_SUCCESS;
                } else {
                    dot_color = CLR_ERROR;
                    is_faulty_pin = true;
                }
                break;
            }
            for (uint8_t inp = 0; inp < ic->gates[g].num_inputs; inp++) {
                if (ic->gates[g].input_pins[inp] == zif_pin) {
                    if (r->gate_results[g].gate_pass) {
                        dot_color = CLR_BG_DARK;
                    } else {
                        dot_color = CLR_ERROR;
                        is_faulty_pin = true;
                    }
                }
            }
        }
        
        if (zif_pin == ic->vcc_pin || zif_pin == ic->gnd_pin) {
            dot_color = CLR_WARNING;
        }
        
        lv_obj_t *pin_dot = lv_obj_create(ic_area);
        lv_obj_set_size(pin_dot, 20, 5);
        lv_obj_align(pin_dot, LV_ALIGN_TOP_MID, is_left ? -54 : 54, py);
        lv_obj_set_style_bg_color(pin_dot, dot_color, 0);
        lv_obj_set_style_border_width(pin_dot, 0, 0);
        lv_obj_set_style_radius(pin_dot, 2, 0);
        lv_obj_clear_flag(pin_dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        
        // ── Blinking Animation for Faulty Gate Pins ──────────────────────────
        if (is_faulty_pin) {
            lv_obj_set_style_shadow_color(pin_dot, CLR_ERROR, 0);
            lv_obj_set_style_shadow_width(pin_dot, 10, 0);
            lv_obj_set_style_shadow_opa(pin_dot, LV_OPA_COVER, 0);

            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, pin_dot);
            lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_20);
            lv_anim_set_time(&a, 350);
            lv_anim_set_playback_time(&a, 350);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
            lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
                lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
                lv_obj_set_style_shadow_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
            });
            lv_anim_start(&a);
        }

        lv_obj_t *lbl_pin = lv_label_create(ic_area);
        lv_label_set_text_fmt(lbl_pin, "%d", zif_pin);
        lv_obj_set_style_text_font(lbl_pin, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_pin, is_faulty_pin ? CLR_ERROR : CLR_TEXT_DIM, 0);
        lv_obj_align(lbl_pin, LV_ALIGN_TOP_MID, is_left ? -76 : 76, py - 4);
    }
    
    // Gate Summary Badge below IC
    lv_obj_t *badge = lv_obj_create(ic_area);
    lv_obj_set_size(badge, 276, 24);
    lv_obj_align(badge, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(badge, (bad_gates > 0) ? CLR_ERROR_DIM : CLR_BLUE_GLOW, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, (bad_gates > 0) ? CLR_ERROR : CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_radius(badge, 12, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl_badge = lv_label_create(badge);
    if (bad_gates > 0) {
        lv_label_set_text_fmt(lbl_badge, LV_SYMBOL_WARNING " GOOD: %d  |  FAULTY: %d", good_gates, bad_gates);
    } else {
        lv_label_set_text_fmt(lbl_badge, LV_SYMBOL_OK " ALL %d GATES PASSED", good_gates);
    }
    lv_obj_set_style_text_font(lbl_badge, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_badge, (bad_gates > 0) ? CLR_ERROR : CLR_TEXT, 0);
    lv_obj_align(lbl_badge, LV_ALIGN_CENTER, 0, 0);
}

// ─── Button callbacks ─────────────────────────────────────────────────────
static void on_home(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    ui_home_show();
}

static void on_retest(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    if (g_result && g_result->ic) {
        ui_test_running_show(g_result->ic, false);
    }
}

static void on_save(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    buzzer_hal_beep_click();
    if (g_result) save_to_history(g_result);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl) lv_label_set_text(lbl, LV_SYMBOL_OK " SAVED!");
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_result_show(const ICTestResult *result) {
    g_result = result;
    bool pass = result->overall_pass;

    scr_result = lv_obj_create(nullptr);
    theme_apply_screen(scr_result);

    // ── TOP HEADER BANNER ───────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_result);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Verdict Badge (Left)
    lv_obj_t *badge_v = lv_obj_create(hdr);
    lv_obj_set_size(badge_v, 140, 28);
    lv_obj_align(badge_v, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_bg_color(badge_v, pass ? CLR_BG_PANEL : CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(badge_v, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge_v, pass ? CLR_BLUE_DIM : CLR_ERROR, 0);
    lv_obj_set_style_border_width(badge_v, 1, 0);
    lv_obj_set_style_radius(badge_v, 14, 0);
    lv_obj_set_style_pad_all(badge_v, 0, 0);
    lv_obj_clear_flag(badge_v, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_verdict = lv_label_create(badge_v);
    lv_label_set_text(lbl_verdict, pass ? LV_SYMBOL_OK " TEST PASSED" : LV_SYMBOL_WARNING " TEST FAILED");
    lv_obj_set_style_text_font(lbl_verdict, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_verdict, pass ? CLR_TEXT : CLR_ERROR, 0);
    lv_obj_align(lbl_verdict, LV_ALIGN_CENTER, 0, 0);

    // IC name + duration (Right) in White
    char hdr_right[28];
    snprintf(hdr_right, sizeof(hdr_right), "%s  |  %lums",
        result->ic->ic_number, result->test_duration_ms);
    lv_obj_t *lbl_hdr_r = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr_r, hdr_right);
    lv_obj_set_style_text_font(lbl_hdr_r, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_hdr_r, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_hdr_r, LV_ALIGN_RIGHT_MID, -12, 0);

    // ── Draw Detailed IC Diagram ────────────────────────────────────────────
    draw_ic_visualization(scr_result, result);

    // ── Bottom Action Buttons (HOME | RETEST | SAVE) ────────────────────────
    lv_obj_t *btn_home = lv_btn_create(scr_result);
    lv_obj_set_size(btn_home, 86, 36);
    lv_obj_align(btn_home, LV_ALIGN_BOTTOM_LEFT, 12, -8);
    theme_apply_btn_secondary(btn_home);
    lv_obj_add_event_cb(btn_home, on_home, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_home = lv_label_create(btn_home);
    lv_label_set_text(lbl_home, LV_SYMBOL_HOME " HOME");
    lv_obj_set_style_text_font(lbl_home, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_home, CLR_TEXT, 0);
    lv_obj_align(lbl_home, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_retest = lv_btn_create(scr_result);
    lv_obj_set_size(btn_retest, 108, 36);
    lv_obj_align(btn_retest, LV_ALIGN_BOTTOM_MID, 0, -8);
    theme_apply_btn(btn_retest);
    lv_obj_add_event_cb(btn_retest, on_retest, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_retest = lv_label_create(btn_retest);
    lv_label_set_text(lbl_retest, LV_SYMBOL_REFRESH " RETEST");
    lv_obj_set_style_text_font(lbl_retest, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_retest, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_retest, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_save = lv_btn_create(scr_result);
    lv_obj_set_size(btn_save, 86, 36);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -12, -8);
    theme_apply_btn_secondary(btn_save);
    lv_obj_add_event_cb(btn_save, on_save, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, LV_SYMBOL_SAVE " SAVE");
    lv_obj_set_style_text_font(lbl_save, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_save, CLR_TEXT, 0);
    lv_obj_align(lbl_save, LV_ALIGN_CENTER, 0, 0);

    // ── Load & trigger sound ────────────────────────────────────────────────
    lv_scr_load_anim(scr_result, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);

    lv_timer_create([](lv_timer_t *t) {
        if (g_result && g_result->overall_pass) buzzer_hal_beep_good();
        else                                    buzzer_hal_beep_bad();
        lv_timer_del(t);
    }, 400, nullptr);
}
