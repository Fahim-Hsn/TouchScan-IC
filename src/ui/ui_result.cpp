/**
 * ui_result.cpp — Detailed IC Test Result Screen
 *
 * Layout (PASS):
 * ┌──────────────────────────────────────────┐
 * │ ✓ 7408  ALL GATES PASSED      32ms       │  Header (green)
 * ├─────────────────────┬────────────────────┤
 * │   IC GATE DIAGRAM   │   TRUTH TABLE       │
 * │                     │  A | B | EXP | ACT │
 * │  [G1 ✓] [G2 ✓]     │  0   0    0    0 ✓ │
 * │  [G3 ✓] [G4 ✓]     │  0   1    0    0 ✓ │
 * │                     │  1   0    0    0 ✓ │
 * │                     │  1   1    1    1 ✓ │
 * ├─────────────────────┴────────────────────┤
 * │   [HOME]         [SAVE HISTORY]          │
 * └──────────────────────────────────────────┘
 *
 * Layout (FAIL):
 *   Header flashes RED, faulty gates shown in red in diagram,
 *   failing truth table rows highlighted red.
 */
#include "ui_result.h"
#include "ui_home.h"
#include "ui_history.h"
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

    // Read current count
    uint8_t cnt = prefs.getUChar("cnt", 0);
    if (cnt >= MAX_HISTORY_ENTRIES) {
        // Shift oldest entry out (simple ring buffer approach)
        for (uint8_t i = 0; i < MAX_HISTORY_ENTRIES - 1; i++) {
            char src[12], dst[12];
            snprintf(src, sizeof(src), "ic%d", i + 1);
            snprintf(dst, sizeof(dst), "ic%d", i);
            // Copy ic_number
            char val[8];
            prefs.getString(src, val, sizeof(val));
            prefs.putString(dst, val);
            // Copy pass/fail
            snprintf(src, sizeof(src), "ok%d", i + 1);
            snprintf(dst, sizeof(dst), "ok%d", i);
            prefs.putBool(dst, prefs.getBool(src, false));
            // Copy time
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
    
    // Create an area for the IC visualization
    lv_obj_t *ic_area = lv_obj_create(parent);
    lv_obj_set_size(ic_area, DISPLAY_WIDTH, 160);
    lv_obj_align(ic_area, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_opa(ic_area, 0, 0); 
    lv_obj_set_style_border_width(ic_area, 0, 0);
    lv_obj_set_style_pad_all(ic_area, 0, 0);
    lv_obj_clear_flag(ic_area, LV_OBJ_FLAG_SCROLLABLE);

    // Physical IC Body (Vertical)
    lv_obj_t *body = lv_obj_create(ic_area);
    lv_obj_set_size(body, 86, 120);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x1A2B3C), 0);
    lv_obj_set_style_border_color(body, lv_color_hex(0x445566), 0);
    lv_obj_set_style_border_width(body, 2, 0);
    lv_obj_set_style_radius(body, 6, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // IC Notch (Top center)
    lv_obj_t *notch = lv_obj_create(body);
    lv_obj_set_size(notch, 20, 20);
    lv_obj_align(notch, LV_ALIGN_TOP_MID, 0, -12);
    lv_obj_set_style_radius(notch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(notch, CLR_BG, 0);
    lv_obj_set_style_border_color(notch, lv_color_hex(0x445566), 0);
    lv_obj_set_style_border_width(notch, 2, 0);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_SCROLLABLE);
    
    // IC Label text inside body
    lv_obj_t *lbl_ic = lv_label_create(body);
    lv_label_set_text_fmt(lbl_ic, "%s\n%s", ic->ic_number, ic->full_name);
    lv_obj_set_style_text_color(lbl_ic, lv_color_hex(0x8899AA), 0); 
    lv_obj_set_style_text_font(lbl_ic, FONT_TINY, 0);
    lv_obj_set_style_text_align(lbl_ic, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_ic, 80);
    lv_obj_align(lbl_ic, LV_ALIGN_CENTER, 0, 0);

    // Count good vs bad gates
    uint8_t good_gates = 0;
    uint8_t bad_gates = 0;
    for (uint8_t g = 0; g < ic->num_gates; g++) {
        if (r->gate_results[g].gate_pass) good_gates++;
        else bad_gates++;
    }

    // Draw Pins and Colored Indicators
    // Each pin creates: 1 pin rect + 1 dot + 1 label = 3 objects
    // For a 14-pin IC that's 42 objects — we combine pin+dot into one object to save
    uint8_t pins_per_side = ic->pin_count / 2;
    uint16_t pin_spacing = 120 / pins_per_side;
    
    for (uint8_t i = 0; i < ic->pin_count; i++) {
        bool is_left = (i < pins_per_side);
        uint8_t row_idx = is_left ? i : (ic->pin_count - 1 - i);
        uint8_t zif_pin = i + 1;
        
        lv_coord_t py = 8 + row_idx * pin_spacing + (pin_spacing / 2) - 2;
        
        // Determine Pin Color
        lv_color_t dot_color = lv_color_hex(0x666666); // Default grey
        bool is_faulty_output = false;
        
        for (uint8_t g = 0; g < ic->num_gates; g++) {
            if (ic->gates[g].output_pin == zif_pin) {
                dot_color = r->gate_results[g].gate_pass ? CLR_SUCCESS : CLR_ERROR;
                is_faulty_output = !r->gate_results[g].gate_pass;
                break;
            }
            for (uint8_t inp = 0; inp < ic->gates[g].num_inputs; inp++) {
                if (ic->gates[g].input_pins[inp] == zif_pin) {
                    dot_color = lv_color_hex(0x00AAFF);
                }
            }
        }
        
        if (zif_pin == ic->vcc_pin || zif_pin == ic->gnd_pin) {
            dot_color = lv_color_hex(0xFF00FF);
        }
        
        // Combined Pin + Dot (single colored rect instead of separate objects)
        lv_obj_t *pin_dot = lv_obj_create(ic_area);
        lv_obj_set_size(pin_dot, 22, 8);
        lv_obj_align(pin_dot, LV_ALIGN_TOP_MID, is_left ? -54 : 54, py);
        lv_obj_set_style_bg_color(pin_dot, dot_color, 0);
        lv_obj_set_style_border_width(pin_dot, 0, 0);
        lv_obj_set_style_radius(pin_dot, 2, 0);
        lv_obj_clear_flag(pin_dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        
        if (is_faulty_output) {
            lv_obj_set_style_shadow_color(pin_dot, CLR_ERROR, 0);
            lv_obj_set_style_shadow_width(pin_dot, 8, 0);
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, pin_dot);
            lv_anim_set_values(&a, 4, 14);
            lv_anim_set_time(&a, 500);
            lv_anim_set_playback_time(&a, 500);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
                lv_obj_set_style_shadow_width((lv_obj_t *)obj, v, 0);
            });
            lv_anim_start(&a);
        }

        // Pin number label
        lv_obj_t *lbl_pin = lv_label_create(ic_area);
        lv_label_set_text_fmt(lbl_pin, "%d", zif_pin);
        lv_obj_set_style_text_font(lbl_pin, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_pin, dot_color, 0);
        lv_obj_align(lbl_pin, LV_ALIGN_TOP_MID, is_left ? -78 : 78, py - 2);
    }
    
    // Gate Summary Badge below IC
    lv_obj_t *badge = lv_obj_create(ic_area);
    lv_obj_set_size(badge, DISPLAY_WIDTH - 30, 24);
    lv_obj_align(badge, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(badge, (bad_gates > 0) ? CLR_ERROR_DIM : CLR_SUCCESS_DIM, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_70, 0);
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
    lv_obj_set_style_text_color(lbl_badge, (bad_gates > 0) ? CLR_ERROR : CLR_SUCCESS, 0);
    lv_obj_align(lbl_badge, LV_ALIGN_CENTER, 0, 0);
}

// ─── Button callbacks ─────────────────────────────────────────────────────
static void on_home(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_home_show();
}
static void on_save(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_result) save_to_history(g_result);
    // Brief feedback
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl) lv_label_set_text(lbl, LV_SYMBOL_OK " SAVED!");
}

// ─── Red flash animation for FAIL ────────────────────────────────────────
static lv_obj_t *flash_overlay = nullptr;
static void do_red_flash(lv_obj_t *scr) {
    if (!scr || lv_scr_act() != scr) return; // Safety: don't flash if screen changed
    // Overlay a red full-screen flash
    flash_overlay = lv_obj_create(scr);
    lv_obj_set_size(flash_overlay, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(flash_overlay, 0, 0);
    lv_obj_set_style_bg_color(flash_overlay, CLR_ERROR, 0);
    lv_obj_set_style_bg_opa(flash_overlay, LV_OPA_40, 0);
    lv_obj_set_style_border_width(flash_overlay, 0, 0);
    lv_obj_clear_flag(flash_overlay, LV_OBJ_FLAG_CLICKABLE);

    // Fade out — use a one-shot timer to delete the flash after the fade
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, flash_overlay);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
    });
    lv_anim_set_values(&a, 100, 0);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    // No deleted_cb — the flash_overlay will be destroyed with its parent screen
    lv_anim_start(&a);
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_result_show(const ICTestResult *result) {
    g_result = result;
    bool pass = result->overall_pass;

    scr_result = lv_obj_create(nullptr);
    theme_apply_screen(scr_result);

    // Skip background grid to save LVGL objects and prevent memory pressure

    // ── Header bar ────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_result);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 36);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, pass ? CLR_SUCCESS_DIM : CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(hdr, pass ? CLR_SUCCESS : CLR_ERROR, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 4, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Result verdict
    char verdict[48];
    snprintf(verdict, sizeof(verdict), "%s  %s",
        pass ? LV_SYMBOL_OK : LV_SYMBOL_WARNING,
        pass ? "ALL GATES PASSED" : "FAULT DETECTED");
    lv_obj_t *lbl_verdict = lv_label_create(hdr);
    lv_label_set_text(lbl_verdict, verdict);
    lv_obj_set_style_text_font(lbl_verdict, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_verdict, pass ? CLR_SUCCESS : CLR_ERROR, 0);
    lv_obj_align(lbl_verdict, LV_ALIGN_LEFT_MID, 6, 0);

    // IC name + time (right)
    char hdr_right[24];
    snprintf(hdr_right, sizeof(hdr_right), "%s | %lums",
        result->ic->ic_number, result->test_duration_ms);
    lv_obj_t *lbl_hdr_r = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr_r, hdr_right);
    lv_obj_set_style_text_font(lbl_hdr_r, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_hdr_r, CLR_TEXT_DIM, 0);
    lv_obj_align(lbl_hdr_r, LV_ALIGN_RIGHT_MID, -4, 0);

    // ── Draw Detailed IC Diagram ─────────────────────────────────────────────
    draw_ic_visualization(scr_result, result);

    // ── Bottom buttons ─────────────────────────────────────────────────────
    lv_obj_t *btn_home = lv_btn_create(scr_result);
    lv_obj_set_size(btn_home, 130, 36);
    lv_obj_align(btn_home, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    theme_apply_btn(btn_home);
    lv_obj_add_event_cb(btn_home, on_home, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_home = lv_label_create(btn_home);
    lv_label_set_text(lbl_home, LV_SYMBOL_HOME "  HOME");
    lv_obj_set_style_text_font(lbl_home, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_home, CLR_NEON, 0);
    lv_obj_align(lbl_home, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_save = lv_btn_create(scr_result);
    lv_obj_set_size(btn_save, 166, 36);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -6, -6);
    theme_apply_btn(btn_save);
    lv_obj_set_style_border_color(btn_save, CLR_SUCCESS, 0);
    lv_obj_add_event_cb(btn_save, on_save, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, LV_SYMBOL_SAVE "  SAVE TO HISTORY");
    lv_obj_set_style_text_font(lbl_save, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_save, CLR_SUCCESS, 0);
    lv_obj_align(lbl_save, LV_ALIGN_CENTER, 0, 0);

    // ── Load & trigger effects ──────────────────────────────────────────────
    lv_scr_load_anim(scr_result, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);

    // Play buzzer after screen has fully loaded (one-shot)
    lv_timer_create([](lv_timer_t *t) {
        if (g_result && g_result->overall_pass) buzzer_hal_beep_good();
        else                                    buzzer_hal_beep_bad();
        lv_timer_del(t);
    }, 500, nullptr);

    // Flash red on fail (delayed to ensure screen is fully loaded)
    if (!pass) {
        lv_timer_create([](lv_timer_t *t) {
            if (scr_result && lv_scr_act() == scr_result) {
                do_red_flash(scr_result);
            }
            lv_timer_del(t);
        }, 500, nullptr);
    }
}
