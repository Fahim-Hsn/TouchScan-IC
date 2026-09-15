/**
 * ui_test_running.cpp — Holographic IC Laser Scanner Screen
 *
 * Features:
 *   - Futuristic Holographic DIP-14/16 IC visualization with realistic pins
 *   - Real-time sweeping neon laser scanner across the chip body
 *   - Active gate pin illumination (inputs & outputs light up during test)
 *   - Live truth table matrix populating gate-by-gate with pass/fail indicators
 *   - Animated scanning progress bar and live duration counter
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

// ─── Shared state between FreeRTOS task and LVGL UI ───────────────────────
static volatile bool         test_done       = false;
static ICTestResult          test_result;
static const ICDescriptor   *test_ic         = nullptr;

// ─── UI Objects ───────────────────────────────────────────────────────────
static lv_obj_t   *scr_test          = nullptr;
static lv_obj_t   *lbl_hdr_title     = nullptr;
static lv_obj_t   *lbl_timer         = nullptr;
static lv_obj_t   *chip_body         = nullptr;
static lv_obj_t   *laser_bar         = nullptr;
static lv_obj_t   *pin_nodes[17]     = {nullptr};
static lv_obj_t   *gate_badges[6]    = {nullptr};
static lv_obj_t   *lbl_active_gate   = nullptr;
static lv_obj_t   *lbl_gate_type     = nullptr;
static lv_obj_t   *row_cont[4]       = {nullptr};
static lv_obj_t   *lbl_row_in[4]     = {nullptr};
static lv_obj_t   *lbl_row_out[4]    = {nullptr};
static lv_obj_t   *lbl_row_status[4] = {nullptr};
static lv_obj_t   *bar_scan          = nullptr;
static lv_obj_t   *lbl_scan_status   = nullptr;
static lv_timer_t *t_ui_anim         = nullptr;

static uint32_t   t_start_ms         = 0;
static uint8_t    displayed_gate     = 0;
static uint32_t   gate_step_ms       = 0;
static bool       scan_completed     = false;

// ─── Laser Animation Callback ─────────────────────────────────────────────
static void anim_laser_y_cb(void *var, int32_t v) {
    if (var) {
        lv_obj_set_y((lv_obj_t *)var, v);
    }
}

// ─── FreeRTOS Test Task ───────────────────────────────────────────────────
static void test_task(void *param) {
    ic_tester_run(test_ic, test_result, nullptr);
    test_done = true;
    vTaskDelete(nullptr);
}

// ─── Clear / Reset Pin Lights ─────────────────────────────────────────────
static void reset_pin_lights(uint8_t pin_count) {
    for (uint8_t p = 1; p <= pin_count; p++) {
        if (pin_nodes[p]) {
            // Default dark pin node
            lv_obj_set_style_bg_color(pin_nodes[p], lv_color_hex(0x064E3B), 0);
            lv_obj_set_style_border_color(pin_nodes[p], lv_color_hex(0x047857), 0);
        }
    }
}

// ─── Highlight Active Gate on Chip & UI ───────────────────────────────────
static void show_gate_data(uint8_t gate_idx) {
    if (!test_ic || gate_idx >= test_ic->num_gates) return;

    const ICGate &gate = test_ic->gates[gate_idx];
    const GateResult &gr = test_result.gate_results[gate_idx];
    uint8_t pc = test_ic->pin_count;

    // 1. Reset all pins
    reset_pin_lights(pc);

    // 2. Light up inputs (Neon Cyan)
    for (uint8_t inp = 0; inp < gate.num_inputs; inp++) {
        uint8_t pin = gate.input_pins[inp];
        if (pin >= 1 && pin <= pc && pin_nodes[pin]) {
            lv_obj_set_style_bg_color(pin_nodes[pin], lv_color_hex(0x38BDF8), 0);
            lv_obj_set_style_border_color(pin_nodes[pin], lv_color_hex(0xBAE6FD), 0);
        }
    }

    // 3. Light up output (Bright Emerald Green if pass, Red if fail)
    if (gate.output_pin >= 1 && gate.output_pin <= pc && pin_nodes[gate.output_pin]) {
        lv_color_t out_clr = gr.gate_pass ? lv_color_hex(0x10B981) : lv_color_hex(0xEF4444);
        lv_obj_set_style_bg_color(pin_nodes[gate.output_pin], out_clr, 0);
        lv_obj_set_style_border_color(pin_nodes[gate.output_pin], lv_color_hex(0xFFFFFF), 0);
    }

    // 4. Update gate badge on left card
    if (gate_badges[gate_idx]) {
        char b_txt[12];
        snprintf(b_txt, sizeof(b_txt), "G%d %s", gate_idx + 1, gr.gate_pass ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
        lv_label_set_text(gate_badges[gate_idx], b_txt);
        lv_obj_set_style_text_color(gate_badges[gate_idx], gr.gate_pass ? lv_color_hex(0x34D399) : lv_color_hex(0xF87171), 0);
    }

    // 5. Update header pill on right card
    if (lbl_active_gate) {
        char g_txt[32];
        snprintf(g_txt, sizeof(g_txt), "GATE %d / %d", gate_idx + 1, test_ic->num_gates);
        lv_label_set_text(lbl_active_gate, g_txt);
    }
    if (lbl_gate_type) {
        lv_label_set_text(lbl_gate_type, ic_tester_gate_type_str(gate.type));
    }

    // 6. Populate Truth Table Rows
    uint8_t num_combos = (gate.num_inputs == 1) ? 2 : (gate.num_inputs == 2 ? 4 : 8);
    for (uint8_t r = 0; r < 4; r++) {
        if (r < num_combos) {
            lv_obj_clear_flag(row_cont[r], LV_OBJ_FLAG_HIDDEN);
            const TTRowResult &row = gr.rows[r];

            char in_txt[16];
            if (gate.num_inputs == 1) {
                snprintf(in_txt, sizeof(in_txt), " [%d] ", row.input_a);
            } else if (gate.num_inputs == 2) {
                snprintf(in_txt, sizeof(in_txt), "[%d,%d]", row.input_a, row.input_b);
            } else {
                snprintf(in_txt, sizeof(in_txt), "[%d%d%d]", row.input_a, row.input_b, row.input_c);
            }
            lv_label_set_text(lbl_row_in[r], in_txt);

            char out_txt[16];
            snprintf(out_txt, sizeof(out_txt), "-> %d", row.actual);
            lv_label_set_text(lbl_row_out[r], out_txt);

            if (row.pass) {
                lv_label_set_text(lbl_row_status[r], LV_SYMBOL_OK " OK");
                lv_obj_set_style_text_color(lbl_row_status[r], CLR_SUCCESS, 0);
                lv_obj_set_style_bg_color(row_cont[r], lv_color_hex(0xE6FDF2), 0);
            } else {
                lv_label_set_text(lbl_row_status[r], LV_SYMBOL_CLOSE " ERR");
                lv_obj_set_style_text_color(lbl_row_status[r], CLR_ERROR, 0);
                lv_obj_set_style_bg_color(row_cont[r], lv_color_hex(0xFEE2E2), 0);
            }
        } else {
            lv_obj_add_flag(row_cont[r], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 7. Update progress bar
    if (bar_scan) {
        int pct = ((gate_idx + 1) * 100) / test_ic->num_gates;
        lv_bar_set_value(bar_scan, pct, LV_ANIM_ON);
    }
    if (lbl_scan_status) {
        char s_txt[40];
        snprintf(s_txt, sizeof(s_txt), "Verifying Gate %d logic matrix...", gate_idx + 1);
        lv_label_set_text(lbl_scan_status, s_txt);
    }
}

// ─── UI Animation & Progression Timer ─────────────────────────────────────
static void on_ui_anim_tick(lv_timer_t *) {
    uint32_t now = millis();

    // Update timer readout in header
    if (lbl_timer) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%lums", now - t_start_ms);
        lv_label_set_text(lbl_timer, buf);
    }

    if (!test_done || !test_ic) return;

    // Stagger gate display (~220ms per gate for high-tech scanning effect)
    if (now - gate_step_ms >= 220 && displayed_gate < test_ic->num_gates) {
        gate_step_ms = now;
        show_gate_data(displayed_gate);
        displayed_gate++;
    }

    // Once all gates displayed and verified
    if (displayed_gate >= test_ic->num_gates && !scan_completed) {
        if (now - gate_step_ms >= 280) {
            scan_completed = true;
            if (bar_scan) lv_bar_set_value(bar_scan, 100, LV_ANIM_ON);
            if (lbl_scan_status) lv_label_set_text(lbl_scan_status, LV_SYMBOL_OK " SCAN COMPLETE");

            // Stop timer & show results
            if (t_ui_anim) {
                lv_timer_del(t_ui_anim);
                t_ui_anim = nullptr;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            ui_result_show(&test_result);
        }
    }
}

// ─── Main Screen Builder ──────────────────────────────────────────────────
void ui_test_running_show(const ICDescriptor *ic, bool auto_detect) {
    test_ic        = ic;
    test_done      = false;
    scan_completed = false;
    displayed_gate = 0;
    t_start_ms     = millis();
    gate_step_ms   = millis();

    // --- Create Screen ---
    scr_test = lv_obj_create(nullptr);
    theme_apply_screen(scr_test);

    // ── Header (h=36) ────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_test);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 36);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Scanner Title with Pulse Icon
    lbl_hdr_title = lv_label_create(hdr);
    char hdr_txt[36];
    snprintf(hdr_txt, sizeof(hdr_txt), LV_SYMBOL_EYE_OPEN "  HOLO-SCAN  %s", ic->ic_number);
    lv_label_set_text(lbl_hdr_title, hdr_txt);
    lv_obj_set_style_text_font(lbl_hdr_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_hdr_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_hdr_title, LV_ALIGN_LEFT_MID, 12, 0);

    // Elapsed Timer Badge
    lv_obj_t *badge_t = lv_obj_create(hdr);
    lv_obj_set_size(badge_t, 64, 22);
    lv_obj_align(badge_t, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(badge_t, CLR_BG_PANEL, 0);
    lv_obj_set_style_border_color(badge_t, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(badge_t, 1, 0);
    lv_obj_set_style_radius(badge_t, 11, 0);
    lv_obj_set_style_pad_all(badge_t, 0, 0);
    lv_obj_clear_flag(badge_t, LV_OBJ_FLAG_SCROLLABLE);

    lbl_timer = lv_label_create(badge_t);
    lv_label_set_text(lbl_timer, "0ms");
    lv_obj_set_style_text_font(lbl_timer, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_timer, CLR_TEXT, 0);
    lv_obj_align(lbl_timer, LV_ALIGN_CENTER, 0, 0);

    // ── Left Card: Holographic Chip Laser Scanner (144x196) ───────────────
    lv_obj_t *card_left = lv_obj_create(scr_test);
    lv_obj_set_size(card_left, 144, 196);
    lv_obj_align(card_left, LV_ALIGN_TOP_LEFT, 8, 38);
    lv_obj_set_style_bg_color(card_left, lv_color_hex(0x022C22), 0); // High-tech deep dark background
    lv_obj_set_style_bg_opa(card_left, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_left, lv_color_hex(0x047857), 0);
    lv_obj_set_style_border_width(card_left, 1, 0);
    lv_obj_set_style_radius(card_left, 12, 0);
    lv_obj_set_style_pad_all(card_left, 4, 0);
    lv_obj_clear_flag(card_left, LV_OBJ_FLAG_SCROLLABLE);

    // IC Body Container (62 x 128)
    chip_body = lv_obj_create(card_left);
    lv_obj_set_size(chip_body, 62, 134);
    lv_obj_align(chip_body, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(chip_body, lv_color_hex(0x011B14), 0);
    lv_obj_set_style_bg_opa(chip_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chip_body, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_border_width(chip_body, 1, 0);
    lv_obj_set_style_radius(chip_body, 6, 0);
    lv_obj_set_style_pad_all(chip_body, 0, 0);
    lv_obj_clear_flag(chip_body, LV_OBJ_FLAG_SCROLLABLE);

    // Top Notch
    lv_obj_t *notch = lv_obj_create(chip_body);
    lv_obj_set_size(notch, 14, 8);
    lv_obj_align(notch, LV_ALIGN_TOP_MID, 0, -4);
    lv_obj_set_style_bg_color(notch, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_border_color(notch, lv_color_hex(0x047857), 0);
    lv_obj_set_style_border_width(notch, 1, 0);
    lv_obj_set_style_radius(notch, 4, 0);

    // Chip Number in Center
    lv_obj_t *lbl_chip = lv_label_create(chip_body);
    lv_label_set_text(lbl_chip, ic->ic_number);
    lv_obj_set_style_text_font(lbl_chip, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_chip, lv_color_hex(0xECFDF5), 0);
    lv_obj_align(lbl_chip, LV_ALIGN_CENTER, 0, -2);

    // Subtitle on Chip
    lv_obj_t *lbl_pkg = lv_label_create(chip_body);
    char pkg_txt[16];
    snprintf(pkg_txt, sizeof(pkg_txt), "DIP-%d", ic->pin_count);
    lv_label_set_text(lbl_pkg, pkg_txt);
    lv_obj_set_style_text_font(lbl_pkg, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_pkg, lv_color_hex(0x059669), 0);
    lv_obj_align(lbl_pkg, LV_ALIGN_CENTER, 0, 14);

    // ── Laser Scanning Line ──────────────────────────────────────────────
    laser_bar = lv_obj_create(chip_body);
    lv_obj_set_size(laser_bar, 58, 2);
    lv_obj_align(laser_bar, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(laser_bar, lv_color_hex(0x34D399), 0);
    lv_obj_set_style_bg_opa(laser_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(laser_bar, 0, 0);
    lv_obj_set_style_shadow_color(laser_bar, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_shadow_width(laser_bar, 8, 0);
    lv_obj_set_style_shadow_spread(laser_bar, 1, 0);
    lv_obj_set_style_shadow_opa(laser_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(laser_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Laser Animation (Up & Down sweep)
    lv_anim_t a_laser;
    lv_anim_init(&a_laser);
    lv_anim_set_var(&a_laser, laser_bar);
    lv_anim_set_values(&a_laser, 8, 122);
    lv_anim_set_time(&a_laser, 650);
    lv_anim_set_playback_time(&a_laser, 650);
    lv_anim_set_repeat_count(&a_laser, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a_laser, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a_laser, anim_laser_y_cb);
    lv_anim_start(&a_laser);

    // ── Build Chip Physical Pins & Glowing LEDs ───────────────────────────
    uint8_t half_pins = ic->pin_count / 2;
    for (uint8_t i = 0; i < half_pins; i++) {
        uint8_t left_pin  = i + 1;
        uint8_t right_pin = ic->pin_count - i;
        int16_t y_pos     = 18 + i * 16;

        // Left Pin Leg (Metal Lead)
        lv_obj_t *leg_l = lv_obj_create(card_left);
        lv_obj_set_size(leg_l, 9, 3);
        lv_obj_set_pos(leg_l, 32, y_pos + 6);
        lv_obj_set_style_bg_color(leg_l, lv_color_hex(0x94A3B8), 0);
        lv_obj_set_style_border_width(leg_l, 0, 0);
        lv_obj_clear_flag(leg_l, LV_OBJ_FLAG_SCROLLABLE);

        // Left Pin LED Dot
        pin_nodes[left_pin] = lv_obj_create(card_left);
        lv_obj_set_size(pin_nodes[left_pin], 7, 7);
        lv_obj_set_pos(pin_nodes[left_pin], 23, y_pos + 4);
        lv_obj_set_style_radius(pin_nodes[left_pin], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(pin_nodes[left_pin], lv_color_hex(0x064E3B), 0);
        lv_obj_set_style_border_color(pin_nodes[left_pin], lv_color_hex(0x047857), 0);
        lv_obj_set_style_border_width(pin_nodes[left_pin], 1, 0);
        lv_obj_clear_flag(pin_nodes[left_pin], LV_OBJ_FLAG_SCROLLABLE);

        // Left Pin Number Label
        lv_obj_t *lbl_lp = lv_label_create(card_left);
        char lp_txt[4];
        snprintf(lp_txt, sizeof(lp_txt), "%d", left_pin);
        lv_label_set_text(lbl_lp, lp_txt);
        lv_obj_set_style_text_font(lbl_lp, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_lp, lv_color_hex(0x6EE7B7), 0);
        lv_obj_set_pos(lbl_lp, 9, y_pos + 1);

        // Right Pin Leg (Metal Lead)
        lv_obj_t *leg_r = lv_obj_create(card_left);
        lv_obj_set_size(leg_r, 9, 3);
        lv_obj_set_pos(leg_r, 103, y_pos + 6);
        lv_obj_set_style_bg_color(leg_r, lv_color_hex(0x94A3B8), 0);
        lv_obj_set_style_border_width(leg_r, 0, 0);
        lv_obj_clear_flag(leg_r, LV_OBJ_FLAG_SCROLLABLE);

        // Right Pin LED Dot
        pin_nodes[right_pin] = lv_obj_create(card_left);
        lv_obj_set_size(pin_nodes[right_pin], 7, 7);
        lv_obj_set_pos(pin_nodes[right_pin], 114, y_pos + 4);
        lv_obj_set_style_radius(pin_nodes[right_pin], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(pin_nodes[right_pin], lv_color_hex(0x064E3B), 0);
        lv_obj_set_style_border_color(pin_nodes[right_pin], lv_color_hex(0x047857), 0);
        lv_obj_set_style_border_width(pin_nodes[right_pin], 1, 0);
        lv_obj_clear_flag(pin_nodes[right_pin], LV_OBJ_FLAG_SCROLLABLE);

        // Right Pin Number Label
        lv_obj_t *lbl_rp = lv_label_create(card_left);
        char rp_txt[4];
        snprintf(rp_txt, sizeof(rp_txt), "%d", right_pin);
        lv_label_set_text(lbl_rp, rp_txt);
        lv_obj_set_style_text_font(lbl_rp, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_rp, lv_color_hex(0x6EE7B7), 0);
        lv_obj_set_pos(lbl_rp, 124, y_pos + 1);
    }

    // ── Bottom Gate Badges on Left Card (G1..G4) ──────────────────────────
    lv_obj_t *cont_badges = lv_obj_create(card_left);
    lv_obj_set_size(cont_badges, 136, 24);
    lv_obj_align(cont_badges, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_opa(cont_badges, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_badges, 0, 0);
    lv_obj_set_style_pad_all(cont_badges, 0, 0);
    lv_obj_clear_flag(cont_badges, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t g = 0; g < ic->num_gates && g < 6; g++) {
        gate_badges[g] = lv_label_create(cont_badges);
        char g_txt[8];
        snprintf(g_txt, sizeof(g_txt), "G%d ..", g + 1);
        lv_label_set_text(gate_badges[g], g_txt);
        lv_obj_set_style_text_font(gate_badges[g], FONT_TINY, 0);
        lv_obj_set_style_text_color(gate_badges[g], lv_color_hex(0x047857), 0);
        int16_t x_offset = (ic->num_gates == 4) ? (g * 34 + 2) : (g * 22 + 2);
        lv_obj_set_pos(gate_badges[g], x_offset, 4);
    }

    // ── Right Card: Live Logic Diagnostic & Truth Table (158x196) ─────────
    lv_obj_t *card_right = lv_obj_create(scr_test);
    lv_obj_set_size(card_right, 158, 196);
    lv_obj_align(card_right, LV_ALIGN_TOP_RIGHT, -8, 38);
    theme_apply_panel(card_right);
    lv_obj_set_style_pad_all(card_right, 6, 0);
    lv_obj_clear_flag(card_right, LV_OBJ_FLAG_SCROLLABLE);

    // Active Gate Header Bar
    lv_obj_t *gate_hdr = lv_obj_create(card_right);
    lv_obj_set_size(gate_hdr, 144, 26);
    lv_obj_align(gate_hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(gate_hdr, CLR_BG_DARK, 0);
    lv_obj_set_style_radius(gate_hdr, 8, 0);
    lv_obj_set_style_border_width(gate_hdr, 0, 0);
    lv_obj_set_style_pad_all(gate_hdr, 0, 0);
    lv_obj_clear_flag(gate_hdr, LV_OBJ_FLAG_SCROLLABLE);

    lbl_active_gate = lv_label_create(gate_hdr);
    lv_label_set_text(lbl_active_gate, "INITIALIZING...");
    lv_obj_set_style_text_font(lbl_active_gate, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_active_gate, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_active_gate, LV_ALIGN_LEFT_MID, 8, 0);

    lbl_gate_type = lv_label_create(gate_hdr);
    lv_label_set_text(lbl_gate_type, "TEST");
    lv_obj_set_style_text_font(lbl_gate_type, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_gate_type, lv_color_hex(0x6EE7B7), 0);
    lv_obj_align(lbl_gate_type, LV_ALIGN_RIGHT_MID, -8, 0);

    // Truth Table Header
    lv_obj_t *tt_head = lv_obj_create(card_right);
    lv_obj_set_size(tt_head, 144, 18);
    lv_obj_align(tt_head, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_opa(tt_head, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tt_head, 0, 0);
    lv_obj_set_style_pad_all(tt_head, 0, 0);
    lv_obj_clear_flag(tt_head, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *th1 = lv_label_create(tt_head);
    lv_label_set_text(th1, "INPUTS");
    lv_obj_set_style_text_font(th1, FONT_TINY, 0);
    lv_obj_set_style_text_color(th1, CLR_TEXT_DIM, 0);
    lv_obj_align(th1, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *th2 = lv_label_create(tt_head);
    lv_label_set_text(th2, "OUT");
    lv_obj_set_style_text_font(th2, FONT_TINY, 0);
    lv_obj_set_style_text_color(th2, CLR_TEXT_DIM, 0);
    lv_obj_align(th2, LV_ALIGN_CENTER, 2, 0);

    lv_obj_t *th3 = lv_label_create(tt_head);
    lv_label_set_text(th3, "RESULT");
    lv_obj_set_style_text_font(th3, FONT_TINY, 0);
    lv_obj_set_style_text_color(th3, CLR_TEXT_DIM, 0);
    lv_obj_align(th3, LV_ALIGN_RIGHT_MID, -4, 0);

    // 4 Dynamic Truth Table Rows
    for (uint8_t r = 0; r < 4; r++) {
        row_cont[r] = lv_obj_create(card_right);
        lv_obj_set_size(row_cont[r], 144, 20);
        lv_obj_align(row_cont[r], LV_ALIGN_TOP_MID, 0, 50 + r * 22);
        lv_obj_set_style_bg_color(row_cont[r], lv_color_hex(0xE6FDF2), 0);
        lv_obj_set_style_border_color(row_cont[r], CLR_BLUE_DIM, 0);
        lv_obj_set_style_border_width(row_cont[r], 1, 0);
        lv_obj_set_style_radius(row_cont[r], 6, 0);
        lv_obj_set_style_pad_all(row_cont[r], 0, 0);
        lv_obj_clear_flag(row_cont[r], LV_OBJ_FLAG_SCROLLABLE);

        lbl_row_in[r] = lv_label_create(row_cont[r]);
        lv_label_set_text(lbl_row_in[r], "[0,0]");
        lv_obj_set_style_text_font(lbl_row_in[r], FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_row_in[r], CLR_TEXT, 0);
        lv_obj_align(lbl_row_in[r], LV_ALIGN_LEFT_MID, 6, 0);

        lbl_row_out[r] = lv_label_create(row_cont[r]);
        lv_label_set_text(lbl_row_out[r], "-> 0");
        lv_obj_set_style_text_font(lbl_row_out[r], FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_row_out[r], CLR_TEXT, 0);
        lv_obj_align(lbl_row_out[r], LV_ALIGN_CENTER, 2, 0);

        lbl_row_status[r] = lv_label_create(row_cont[r]);
        lv_label_set_text(lbl_row_status[r], LV_SYMBOL_OK " OK");
        lv_obj_set_style_text_font(lbl_row_status[r], FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_row_status[r], CLR_SUCCESS, 0);
        lv_obj_align(lbl_row_status[r], LV_ALIGN_RIGHT_MID, -6, 0);
    }

    // Scanning Status & Progress Bar (Bottom)
    lbl_scan_status = lv_label_create(card_right);
    lv_label_set_text(lbl_scan_status, "Scanning truth matrix...");
    lv_obj_set_style_text_font(lbl_scan_status, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_scan_status, CLR_TEXT_DIM, 0);
    lv_obj_align(lbl_scan_status, LV_ALIGN_BOTTOM_LEFT, 2, -14);

    bar_scan = lv_bar_create(card_right);
    lv_obj_set_size(bar_scan, 144, 6);
    lv_obj_align(bar_scan, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(bar_scan, lv_color_hex(0xCCFBF1), 0);
    lv_obj_set_style_bg_color(bar_scan, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_scan, 3, 0);
    lv_obj_set_style_radius(bar_scan, 3, LV_PART_INDICATOR);
    lv_bar_set_range(bar_scan, 0, 100);
    lv_bar_set_value(bar_scan, 5, LV_ANIM_OFF);

    // ── Load Screen with Clean Transition ────────────────────────────────
    lv_scr_load_anim(scr_test, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);

    // ── Start FreeRTOS Hardware Test Task ─────────────────────────────────
    xTaskCreatePinnedToCore(
        test_task,
        "ICTest",
        8192,
        nullptr,
        5,
        nullptr,
        0
    );

    // ── Start UI Refresh Timer (every 30ms) ───────────────────────────────
    t_ui_anim = lv_timer_create(on_ui_anim_tick, 30, nullptr);
}
