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
    
    // Create an area for the IC visualization (centered)
    lv_obj_t *ic_area = lv_obj_create(parent);
    lv_obj_set_size(ic_area, DISPLAY_WIDTH - 20, 150);
    lv_obj_align(ic_area, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_opa(ic_area, 0, 0); 
    lv_obj_set_style_border_width(ic_area, 0, 0);
    lv_obj_clear_flag(ic_area, LV_OBJ_FLAG_SCROLLABLE);

    // Physical IC Body
    lv_obj_t *body = lv_obj_create(ic_area);
    lv_obj_set_size(body, 220, 80);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x111111), 0); // Dark grey
    lv_obj_set_style_border_color(body, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(body, 2, 0);
    lv_obj_set_style_radius(body, 8, 0);
    lv_obj_set_style_shadow_color(body, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(body, 20, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // IC Notch (Orientation mark)
    lv_obj_t *notch = lv_obj_create(body);
    lv_obj_set_size(notch, 20, 30);
    lv_obj_align(notch, LV_ALIGN_LEFT_MID, -10, 0);
    lv_obj_set_style_radius(notch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(notch, CLR_BG, 0);
    lv_obj_set_style_border_color(notch, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(notch, 2, 0);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_SCROLLABLE);
    
    // IC Label text
    lv_obj_t *lbl_ic = lv_label_create(body);
    char ic_lbl[32];
    snprintf(ic_lbl, sizeof(ic_lbl), "%s", ic->ic_number);
    lv_label_set_text(lbl_ic, ic_lbl);
    lv_obj_set_style_text_color(lbl_ic, lv_color_hex(0x555555), 0); 
    lv_obj_set_style_text_font(lbl_ic, FONT_SMALL, 0);
    lv_obj_align(lbl_ic, LV_ALIGN_LEFT_MID, 25, -20);

    // Draw Pins
    uint8_t pins_per_side = ic->pin_count / 2;
    uint16_t pin_spacing = 180 / pins_per_side;
    uint16_t start_x = 20 + (180 - (pins_per_side - 1) * pin_spacing) / 2;
    
    for (uint8_t i = 0; i < ic->pin_count; i++) {
        bool is_bottom = (i < pins_per_side);
        uint8_t pin_idx = is_bottom ? i : (ic->pin_count - 1 - i);
        
        lv_obj_t *pin = lv_obj_create(ic_area);
        lv_obj_set_size(pin, 10, 16);
        lv_coord_t px = (lv_coord_t)(start_x + pin_idx * pin_spacing + (300 - 220)/2 - 10);
        lv_coord_t py = is_bottom ? (150/2 + 40) : (150/2 - 40 - 16);
        lv_obj_align(pin, LV_ALIGN_TOP_LEFT, px, py);
        
        lv_obj_set_style_bg_color(pin, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_bg_grad_color(pin, lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_grad_dir(pin, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_border_width(pin, 1, 0);
        lv_obj_set_style_border_color(pin, lv_color_hex(0x222222), 0);
        lv_obj_clear_flag(pin, LV_OBJ_FLAG_SCROLLABLE);

        // Pin numbers
        lv_obj_t *lbl_pin = lv_label_create(ic_area);
        char pnum[4];
        snprintf(pnum, sizeof(pnum), "%d", i + 1);
        lv_label_set_text(lbl_pin, pnum);
        lv_obj_set_style_text_font(lbl_pin, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_pin, CLR_TEXT_DIM, 0);
        lv_obj_align(lbl_pin, LV_ALIGN_TOP_LEFT, px, is_bottom ? (py + 18) : (py - 14));
    }

    // Draw Gates inside the IC body
    uint8_t num_gates = ic->num_gates;
    uint8_t cols = (num_gates <= 4) ? num_gates : ((num_gates + 1) / 2);
    uint8_t rows = (num_gates <= 4) ? 1 : 2;
    
    uint16_t gate_w = 40;
    uint16_t gate_h = 30;
    uint16_t spacing_x = (220 - 40) / cols;
    uint16_t spacing_y = 80 / rows;
    
    for (uint8_t g = 0; g < num_gates; g++) {
        const GateResult &gr = r->gate_results[g];
        uint8_t c = g % cols;
        uint8_t row = g / cols;
        
        lv_obj_t *g_box = lv_obj_create(body);
        lv_obj_set_size(g_box, gate_w, gate_h);
        
        lv_coord_t x = 20 + c * spacing_x + (spacing_x - gate_w) / 2;
        lv_coord_t y = row * spacing_y + (spacing_y - gate_h) / 2;
        lv_obj_align(g_box, LV_ALIGN_TOP_LEFT, x, y);
        
        // Style depending on pass/fail
        if (gr.gate_pass) {
            lv_obj_set_style_bg_color(g_box, CLR_SUCCESS_DIM, 0);
            lv_obj_set_style_bg_opa(g_box, LV_OPA_50, 0);
            lv_obj_set_style_border_color(g_box, CLR_SUCCESS, 0);
            lv_obj_set_style_shadow_color(g_box, CLR_SUCCESS, 0);
            lv_obj_set_style_shadow_width(g_box, 15, 0);
        } else {
            lv_obj_set_style_bg_color(g_box, CLR_ERROR_DIM, 0);
            lv_obj_set_style_bg_opa(g_box, LV_OPA_80, 0);
            lv_obj_set_style_border_color(g_box, CLR_ERROR, 0);
            lv_obj_set_style_shadow_color(g_box, CLR_ERROR, 0);
            lv_obj_set_style_shadow_width(g_box, 25, 0);
            
            // Pulse animation
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, g_box);
            lv_anim_set_values(&a, 10, 35);
            lv_anim_set_time(&a, 600);
            lv_anim_set_playback_time(&a, 600);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
                lv_obj_set_style_shadow_width((lv_obj_t *)obj, v, 0);
            });
            lv_anim_start(&a);
        }
        
        lv_obj_set_style_border_width(g_box, 2, 0);
        lv_obj_set_style_radius(g_box, 4, 0);
        lv_obj_clear_flag(g_box, LV_OBJ_FLAG_SCROLLABLE);
        
        // Gate label (e.g. "G1 AND")
        lv_obj_t *lbl = lv_label_create(g_box);
        char buf[16];
        snprintf(buf, sizeof(buf), "G%d\n%s", gr.gate_id, ic_tester_gate_type_str(gr.type));
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl, gr.gate_pass ? CLR_SUCCESS : CLR_ERROR, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
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
static void do_red_flash(lv_obj_t *scr) {
    // Overlay a red full-screen flash
    lv_obj_t *flash = lv_obj_create(scr);
    lv_obj_set_size(flash, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(flash, 0, 0);
    lv_obj_set_style_bg_color(flash, CLR_ERROR, 0);
    lv_obj_set_style_bg_opa(flash, LV_OPA_50, 0);
    lv_obj_set_style_border_width(flash, 0, 0);
    lv_obj_clear_flag(flash, LV_OBJ_FLAG_CLICKABLE);

    // Fade out the flash
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, flash);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
    });
    lv_anim_set_values(&a, 130, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_deleted_cb(&a, [](lv_anim_t *an) {
        lv_obj_del(static_cast<lv_obj_t *>(an->var));
    });
    lv_anim_start(&a);
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_result_show(const ICTestResult *result) {
    g_result = result;
    bool pass = result->overall_pass;

    scr_result = lv_obj_create(nullptr);
    theme_apply_screen(scr_result);

    // Background grid
    for (int i = 0; i < DISPLAY_HEIGHT / 20; i++) {
        lv_obj_t *g = lv_obj_create(scr_result);
        lv_obj_set_size(g, DISPLAY_WIDTH, 1);
        lv_obj_set_pos(g, 0, i * 20);
        lv_obj_set_style_bg_color(g, CLR_GRID, 0);
        lv_obj_set_style_bg_opa(g, LV_OPA_30, 0);
        lv_obj_set_style_border_width(g, 0, 0);
        lv_obj_clear_flag(g, LV_OBJ_FLAG_CLICKABLE);
    }

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

    // Play buzzer after screen appears (one-shot)
    lv_timer_create([](lv_timer_t *t) {
        if (g_result && g_result->overall_pass) buzzer_hal_beep_good();
        else                                    buzzer_hal_beep_bad();
        lv_timer_del(t);
    }, 350, nullptr);

    // Flash red on fail
    if (!pass) {
        lv_timer_create([](lv_timer_t *t) {
            do_red_flash(scr_result);
            lv_timer_del(t);
        }, 300, nullptr);
    }
}
