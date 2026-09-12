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

// ─── Gate diagram drawing (on LVGL canvas) ────────────────────────────────
static void draw_gate_diagram(lv_obj_t *canvas, const ICTestResult *r) {
    // Canvas buffer
    static lv_color_t cbuf[150 * 150];
    lv_canvas_set_buffer(canvas, cbuf, 150, 150, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, CLR_BG_PANEL, LV_OPA_COVER);

    const ICDescriptor *ic = r->ic;
    uint8_t ng = ic->num_gates;

    // Arrange gates in a grid: up to 4 per row, max 2 rows
    uint8_t cols = (ng <= 3) ? ng : 2;
    uint8_t rows = (ng + cols - 1) / cols;

    uint16_t cell_w = 150 / cols;
    uint16_t cell_h = (ng <= 4) ? 70 : 35;
    uint16_t gate_w = cell_w - 10;
    uint16_t gate_h = (ng <= 4) ? 44 : 26;

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_label_dsc_t lbl_dsc;

    for (uint8_t g = 0; g < ng; g++) {
        const GateResult &gr = r->gate_results[g];
        uint8_t col = g % cols;
        uint8_t row = g / cols;

        lv_coord_t x = col * cell_w + (cell_w - gate_w) / 2;
        lv_coord_t y = 12 + row * cell_h;

        // Gate box
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color    = gr.gate_pass ? CLR_SUCCESS_DIM : CLR_ERROR_DIM;
        rect_dsc.bg_opa      = LV_OPA_50;
        rect_dsc.border_color = gr.gate_pass ? CLR_SUCCESS : CLR_ERROR;
        rect_dsc.border_width = 2;
        rect_dsc.radius      = 4;
        lv_canvas_draw_rect(canvas, x, y, gate_w, gate_h, &rect_dsc);

        // Gate type label
        lv_draw_label_dsc_init(&lbl_dsc);
        lbl_dsc.color = gr.gate_pass ? CLR_SUCCESS : CLR_ERROR;
        lbl_dsc.font  = FONT_TINY;
        char gt_str[10];
        snprintf(gt_str, sizeof(gt_str), "%s", ic_tester_gate_type_str(gr.type));
        lv_canvas_draw_text(canvas, x + 4, y + 4, gate_w - 8, &lbl_dsc, gt_str);

        // Gate number + status
        lbl_dsc.color = CLR_TEXT;
        char gate_id[8];
        snprintf(gate_id, sizeof(gate_id), "G%d %s", gr.gate_id, gr.gate_pass ? "\xE2\x9C\x93" : "X");
        if (ng <= 4) {
            lv_canvas_draw_text(canvas, x + 4, y + gate_h - 14, gate_w - 8, &lbl_dsc, gate_id);
        }

        // Draw simplified pin lines
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = gr.gate_pass ? CLR_SUCCESS : CLR_ERROR;
        line_dsc.width = 1;
        line_dsc.opa   = LV_OPA_60;

        // Input lines (left side)
        if (ng <= 4) {
            lv_point_t pts_in1[2] = {{(lv_coord_t)(x - 8), (lv_coord_t)(y + gate_h/3)},
                                      {(lv_coord_t)x,       (lv_coord_t)(y + gate_h/3)}};
            lv_canvas_draw_line(canvas, pts_in1, 2, &line_dsc);
            if (ic->gates[g].num_inputs > 1) {
                lv_point_t pts_in2[2] = {{(lv_coord_t)(x - 8), (lv_coord_t)(y + 2*gate_h/3)},
                                          {(lv_coord_t)x,       (lv_coord_t)(y + 2*gate_h/3)}};
                lv_canvas_draw_line(canvas, pts_in2, 2, &line_dsc);
            }
            // Output line (right side)
            lv_point_t pts_out[2] = {{(lv_coord_t)(x + gate_w),     (lv_coord_t)(y + gate_h/2)},
                                      {(lv_coord_t)(x + gate_w + 8), (lv_coord_t)(y + gate_h/2)}};
            lv_canvas_draw_line(canvas, pts_out, 2, &line_dsc);
        }
    }

    // IC label at top
    lv_draw_label_dsc_init(&lbl_dsc);
    lbl_dsc.color = CLR_NEON;
    lbl_dsc.font  = FONT_TINY;
    lv_canvas_draw_text(canvas, 2, 0, 146, &lbl_dsc, ic->ic_number);
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

    // ── Left column — Gate diagram ─────────────────────────────────────────
    lv_obj_t *canvas = lv_canvas_create(scr_result);
    lv_obj_set_size(canvas, 150, 150);
    lv_obj_align(canvas, LV_ALIGN_LEFT_MID, 4, 8);
    draw_gate_diagram(canvas, result);

    // Fault message below diagram (if FAIL)
    if (!pass) {
        // Find first failing gate
        for (uint8_t g = 0; g < result->num_gates; g++) {
            if (!result->gate_results[g].gate_pass) {
                lv_obj_t *lbl_fault = lv_label_create(scr_result);
                char fault_msg[64];
                const GateResult &gr = result->gate_results[g];
                snprintf(fault_msg, sizeof(fault_msg),
                    "GATE %d FAIL\nPIN %d OUT\nEXP:%d GOT:%d",
                    gr.gate_id, gr.output_pin,
                    gr.rows[0].expected, gr.rows[0].actual);
                lv_label_set_text(lbl_fault, fault_msg);
                lv_obj_set_style_text_font(lbl_fault, FONT_TINY, 0);
                lv_obj_set_style_text_color(lbl_fault, CLR_ERROR, 0);
                lv_label_set_long_mode(lbl_fault, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(lbl_fault, 148);
                lv_obj_align(lbl_fault, LV_ALIGN_LEFT_MID, 4, 70);
                break;
            }
        }
    }

    // ── Right column — Truth table ─────────────────────────────────────────
    lv_obj_t *tbl = lv_table_create(scr_result);
    lv_obj_set_size(tbl, 158, DISPLAY_HEIGHT - 36 - 50);
    lv_obj_align(tbl, LV_ALIGN_RIGHT_MID, -4, -14);
    lv_table_set_col_cnt(tbl, 5);
    lv_table_set_col_width(tbl, 0, 22);  // Gate#
    lv_table_set_col_width(tbl, 1, 22);  // A
    lv_table_set_col_width(tbl, 2, 22);  // B
    lv_table_set_col_width(tbl, 3, 42);  // EXP
    lv_table_set_col_width(tbl, 4, 42);  // ACT

    lv_obj_set_style_bg_color(tbl, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(tbl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tbl, CLR_BLUE_DIM, 0);
    lv_obj_set_style_text_font(tbl, FONT_TINY, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tbl, CLR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(tbl, CLR_BG_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(tbl, 1, LV_PART_ITEMS);

    // Header
    lv_table_set_cell_value(tbl, 0, 0, "G");
    lv_table_set_cell_value(tbl, 0, 1, "A");
    lv_table_set_cell_value(tbl, 0, 2, "B");
    lv_table_set_cell_value(tbl, 0, 3, "EXP");
    lv_table_set_cell_value(tbl, 0, 4, "ACT");

    uint16_t row = 1;
    for (uint8_t g = 0; g < result->num_gates; g++) {
        const GateResult &gr = result->gate_results[g];
        for (uint8_t r2 = 0; r2 < gr.num_rows; r2++) {
            const TTRowResult &tr = gr.rows[r2];
            char buf[8];

            snprintf(buf, sizeof(buf), "%d", gr.gate_id);
            lv_table_set_cell_value(tbl, row, 0, buf);
            snprintf(buf, sizeof(buf), "%d", tr.input_a);
            lv_table_set_cell_value(tbl, row, 1, buf);
            snprintf(buf, sizeof(buf), "%d", tr.input_b);
            lv_table_set_cell_value(tbl, row, 2, buf);
            snprintf(buf, sizeof(buf), "%d", tr.expected);
            lv_table_set_cell_value(tbl, row, 3, buf);
            snprintf(buf, sizeof(buf), tr.pass ? "%d \xE2\x9C\x93" : "%d X", tr.actual);
            lv_table_set_cell_value(tbl, row, 4, buf);
            row++;
        }
    }

    // Draw event to colour failing rows red
    lv_obj_add_event_cb(tbl, [](lv_event_t *ev) {
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(ev);
        if (!dsc || dsc->part != LV_PART_ITEMS) return;
        if (!g_result) return;

        // Map table row → gate + combo
        uint16_t tbl_row = dsc->id / 5;  // col count is 5 (id = row*col_cnt + col)
        if (tbl_row == 0) return;
        uint16_t data_row = tbl_row - 1;

        // Find which gate/row this is
        uint16_t idx = 0;
        for (uint8_t g = 0; g < g_result->num_gates; g++) {
            for (uint8_t r2 = 0; r2 < g_result->gate_results[g].num_rows; r2++) {
                if (idx == data_row) {
                    if (!g_result->gate_results[g].rows[r2].pass) {
                        dsc->rect_dsc->bg_color = CLR_ERROR_DIM;
                        dsc->rect_dsc->bg_opa   = LV_OPA_50;
                        dsc->label_dsc->color   = CLR_ERROR;
                    } else {
                        dsc->label_dsc->color = CLR_SUCCESS;
                    }
                    return;
                }
                idx++;
            }
        }
    }, LV_EVENT_DRAW_PART_BEGIN, nullptr);

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
