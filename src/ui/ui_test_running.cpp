/**
 * ui_test_running.cpp — Animated Test-In-Progress Screen
 *
 * Shows:
 *   - IC name in header
 *   - Spinning arc (LVGL spinner) during test
 *   - Gate status: "Testing Gate X / N..."
 *   - Truth table rows populating in real-time
 *   - Test duration counter
 *
 * Note: The actual IC test runs in a FreeRTOS task so the LVGL
 *       main loop (lv_task_handler) keeps rendering animations.
 */
#include "ui_test_running.h"
#include "ui_result.h"
#include "ui_theme.h"
#include "../engine/ic_tester.h"
#include "../config.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

// ─── Shared state between FreeRTOS task and LVGL main loop ───────────────
static volatile bool         test_done       = false;
static ICTestResult          test_result;
static const ICDescriptor   *test_ic         = nullptr;
static volatile uint8_t      current_gate    = 0;
static volatile uint8_t      total_gates     = 0;

// ─── UI elements ──────────────────────────────────────────────────────────
static lv_obj_t *scr_test       = nullptr;
static lv_obj_t *lbl_gate_info  = nullptr;
static lv_obj_t *lbl_timer      = nullptr;
static lv_obj_t *tbl_tt         = nullptr;  // Truth table widget
static lv_timer_t *t_poll       = nullptr;
static uint32_t   t_start_ms    = 0;

// ─── Progress callback (called from FreeRTOS task!) ──────────────────────
// IMPORTANT: This runs in a non-LVGL task — only update volatile state.
static void on_test_progress(uint8_t gate_idx, uint8_t total) {
    current_gate = gate_idx;
    total_gates  = total;
}

// ─── FreeRTOS test task ───────────────────────────────────────────────────
static void test_task(void *param) {
    ic_tester_run(test_ic, test_result, on_test_progress);
    test_done = true;
    vTaskDelete(nullptr);
}

// ─── LVGL poll timer (checks if test is done) ────────────────────────────
static void poll_test_complete(lv_timer_t *) {
    // Update the elapsed time display
    if (lbl_timer) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%lums", millis() - t_start_ms);
        lv_label_set_text(lbl_timer, buf);
    }

    // Update gate status
    if (lbl_gate_info && total_gates > 0) {
        char buf[40];
        snprintf(buf, sizeof(buf), "Testing Gate %d / %d", current_gate, total_gates);
        lv_label_set_text(lbl_gate_info, buf);
    }

    // Check if test is complete
    if (test_done) {
        lv_timer_del(t_poll);
        t_poll = nullptr;
        // Navigate to result screen
        ui_result_show(&test_result);
    }
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_test_running_show(const ICDescriptor *ic, bool auto_detect) {
    test_ic      = ic;
    test_done    = false;
    current_gate = 0;
    total_gates  = ic->num_gates;
    t_start_ms   = millis();

    // --- Create screen ---
    scr_test = lv_obj_create(nullptr);
    theme_apply_screen(scr_test);

    // Background grid
    for (int i = 0; i < DISPLAY_HEIGHT / 20; i++) {
        lv_obj_t *g = lv_obj_create(scr_test);
        lv_obj_set_size(g, DISPLAY_WIDTH, 1);
        lv_obj_set_pos(g, 0, i * 20);
        lv_obj_set_style_bg_color(g, CLR_GRID, 0);
        lv_obj_set_style_bg_opa(g, LV_OPA_30, 0);
        lv_obj_set_style_border_width(g, 0, 0);
        lv_obj_clear_flag(g, LV_OBJ_FLAG_CLICKABLE);
    }

    // ── Header ──────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_test);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, CLR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(hdr, CLR_SEPARATOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 4, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // IC name in header
    char hdr_text[32];
    snprintf(hdr_text, sizeof(hdr_text), "TESTING  %s", ic->ic_number);
    lv_obj_t *lbl_hdr = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr, hdr_text);
    lv_obj_set_style_text_font(lbl_hdr, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_hdr, CLR_NEON, 0);
    lv_obj_set_style_text_letter_space(lbl_hdr, 1, 0);
    lv_obj_align(lbl_hdr, LV_ALIGN_CENTER, 0, 0);

    // Timer label (right)
    lbl_timer = lv_label_create(hdr);
    lv_label_set_text(lbl_timer, "0ms");
    lv_obj_set_style_text_font(lbl_timer, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_timer, CLR_TEXT_LABEL, 0);
    lv_obj_align(lbl_timer, LV_ALIGN_RIGHT_MID, -4, 0);

    // ── Centre area ──────────────────────────────────────────────────────
    // Left: Spinner + gate info
    lv_obj_t *spin = lv_spinner_create(scr_test, 1500, 60);
    lv_obj_set_size(spin, 70, 70);
    lv_obj_align(spin, LV_ALIGN_LEFT_MID, 20, -10);
    lv_obj_set_style_arc_color(spin, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spin, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spin, CLR_BG_PANEL, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spin, 4, LV_PART_MAIN);

    // Gate info label
    lbl_gate_info = lv_label_create(scr_test);
    lv_label_set_text(lbl_gate_info, "Starting test...");
    lv_obj_set_style_text_font(lbl_gate_info, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_gate_info, CLR_TEXT, 0);
    lv_obj_set_style_text_align(lbl_gate_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_gate_info, 110);
    lv_obj_align(lbl_gate_info, LV_ALIGN_LEFT_MID, 8, 36);

    // Right: Truth table (populates during test)
    tbl_tt = lv_table_create(scr_test);
    lv_obj_set_size(tbl_tt, 160, DISPLAY_HEIGHT - 40);
    lv_obj_align(tbl_tt, LV_ALIGN_RIGHT_MID, -4, 8);
    lv_table_set_col_cnt(tbl_tt, 4);
    lv_table_set_col_width(tbl_tt, 0, 30);
    lv_table_set_col_width(tbl_tt, 1, 30);
    lv_table_set_col_width(tbl_tt, 2, 40);
    lv_table_set_col_width(tbl_tt, 3, 40);

    // Header row
    lv_table_set_cell_value(tbl_tt, 0, 0, "A");
    lv_table_set_cell_value(tbl_tt, 0, 1, "B");
    lv_table_set_cell_value(tbl_tt, 0, 2, "EXP");
    lv_table_set_cell_value(tbl_tt, 0, 3, "ACT");

    // Style table
    lv_obj_set_style_bg_color(tbl_tt, CLR_BG_PANEL, 0);
    lv_obj_set_style_border_color(tbl_tt, CLR_BLUE_DIM, 0);
    lv_obj_set_style_text_font(tbl_tt, FONT_TINY, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tbl_tt, CLR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(tbl_tt, CLR_BG_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_border_color(tbl_tt, CLR_SEPARATOR, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(tbl_tt, 2, LV_PART_ITEMS);

    // Load screen
    lv_scr_load_anim(scr_test, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, true);

    // ── Start the test in a FreeRTOS task ────────────────────────────────
    xTaskCreate(
        test_task,
        "ICTest",
        4096,   // Stack size (4KB)
        nullptr,
        5,      // High priority
        nullptr
    );

    // ── Poll timer to update UI and check completion ──────────────────────
    t_poll = lv_timer_create(poll_test_complete, 50, nullptr);  // 50ms = 20fps update
}
