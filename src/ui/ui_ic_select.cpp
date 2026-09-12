/**
 * ui_ic_select.cpp — Scrollable IC Browser
 *
 * Layout:
 * ┌─────────────────────────────────────┐
 * │ ← BACK     SELECT IC     [7408] ↓  │  Header
 * ├─────────────────────────────────────┤
 * │  ┌──────────────────────────────┐   │
 * │  │ 7400  Quad NAND     [  TEST ]│   │  ← List items
 * │  │ 7402  Quad NOR      [  TEST ]│   │
 * │  │ 7404  Hex NOT       [  TEST ]│   │
 * │  │ 7408  Quad AND      [  TEST ]│   │  (selected, neon border)
 * │  │ 7432  Quad OR       [  TEST ]│   │
 * │  │ 7486  Quad XOR      [  TEST ]│   │
 * │  └──────────────────────────────┘   │
 * │                                     │
 * │    [──── TEST SELECTED IC ────]     │  Action button
 * └─────────────────────────────────────┘
 */
#include "ui_ic_select.h"
#include "ui_test_running.h"
#include "ui_home.h"
#include "ui_theme.h"
#include "../engine/ic_database.h"
#include "../hal/zif_hal.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t       *scr_select    = nullptr;
static lv_obj_t       *lbl_selected  = nullptr;
static const ICDescriptor *selected_ic = nullptr;

static void on_back(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_home_show();
}

static void on_ic_item_click(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    selected_ic = ic_db_get(idx);

    if (selected_ic && lbl_selected) {
        lv_label_set_text(lbl_selected, selected_ic->ic_number);
    }
}

// ─── Warning popup ────────────────────────────────────────────────────────
static lv_obj_t *warn_box = nullptr;

static void close_warning(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (warn_box) {
        lv_obj_del(warn_box);
        warn_box = nullptr;
    }
}

static void show_no_ic_warning(void) {
    if (warn_box) return; // Already showing
    
    buzzer_hal_beep_bad();
    
    // Dark overlay
    warn_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(warn_box, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(warn_box, 0, 0);
    lv_obj_set_style_bg_color(warn_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(warn_box, LV_OPA_60, 0);
    lv_obj_set_style_border_width(warn_box, 0, 0);
    lv_obj_clear_flag(warn_box, LV_OBJ_FLAG_SCROLLABLE);
    
    // Warning card
    lv_obj_t *card = lv_obj_create(warn_box);
    lv_obj_set_size(card, 260, 120);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, CLR_BG_PANEL, 0);
    lv_obj_set_style_border_color(card, CLR_ERROR, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_shadow_color(card, CLR_ERROR, 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Warning icon + text
    lv_obj_t *lbl_warn = lv_label_create(card);
    lv_label_set_text(lbl_warn, LV_SYMBOL_WARNING "  NO IC DETECTED!");
    lv_obj_set_style_text_font(lbl_warn, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_warn, CLR_ERROR, 0);
    lv_obj_align(lbl_warn, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *lbl_msg = lv_label_create(card);
    lv_label_set_text(lbl_msg, "Insert IC into ZIF socket\nbefore testing.");
    lv_obj_set_style_text_font(lbl_msg, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_msg, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, 4);
    
    // OK button
    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_size(btn_ok, 80, 28);
    lv_obj_align(btn_ok, LV_ALIGN_BOTTOM_MID, 0, -6);
    theme_apply_btn(btn_ok);
    lv_obj_set_style_border_color(btn_ok, CLR_NEON, 0);
    lv_obj_add_event_cb(btn_ok, close_warning, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "OK");
    lv_obj_set_style_text_font(lbl_ok, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_ok, CLR_NEON, 0);
    lv_obj_align(lbl_ok, LV_ALIGN_CENTER, 0, 0);
    
    // Also close on tapping overlay background
    lv_obj_add_event_cb(warn_box, close_warning, LV_EVENT_ALL, nullptr);
}

static void on_test_selected(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!selected_ic) return;
    
    // Check if IC is actually inserted in ZIF socket
    if (!zif_hal_detect_ic_presence()) {
        show_no_ic_warning();
        return;
    }
    
    ui_test_running_show(selected_ic, false);
}

void ui_ic_select_show(void) {
    scr_select = lv_obj_create(nullptr);
    theme_apply_screen(scr_select);

    // ── Header bar ──────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_select);
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

    // Back button
    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 55, 22);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
    theme_apply_btn(btn_back);
    lv_obj_add_event_cb(btn_back, on_back, LV_EVENT_ALL, nullptr);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_font(lbl_back, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_back, CLR_NEON, 0);
    lv_obj_align(lbl_back, LV_ALIGN_CENTER, 0, 0);

    // Title
    lv_obj_t *lbl_hdr_title = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr_title, "SELECT IC");
    lv_obj_set_style_text_font(lbl_hdr_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_hdr_title, CLR_NEON, 0);
    lv_obj_set_style_text_letter_space(lbl_hdr_title, 2, 0);
    lv_obj_align(lbl_hdr_title, LV_ALIGN_CENTER, 0, 0);

    // Selected IC indicator
    lbl_selected = lv_label_create(hdr);
    lv_label_set_text(lbl_selected, "---");
    lv_obj_set_style_text_font(lbl_selected, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_selected, CLR_WARNING, 0);
    lv_obj_align(lbl_selected, LV_ALIGN_RIGHT_MID, -4, 0);

    // ── Scrollable IC list ──────────────────────────────────────────────
    lv_obj_t *list = lv_obj_create(scr_select);
    lv_obj_set_size(list, DISPLAY_WIDTH - 12, DISPLAY_HEIGHT - 30 - 50);
    lv_obj_set_pos(list, 6, 34);
    lv_obj_set_style_bg_color(list, CLR_BG, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_row(list, 4, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    uint8_t count = ic_db_count();
    for (uint8_t i = 0; i < count; i++) {
        const ICDescriptor *ic = ic_db_get(i);

        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), 38);
        lv_obj_set_style_bg_color(row, CLR_BG_PANEL, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, CLR_BLUE_DIM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row, on_ic_item_click, LV_EVENT_ALL, (void *)(uintptr_t)i);

        // Pressed state
        lv_obj_set_style_bg_color(row, CLR_BLUE_GLOW, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(row, CLR_NEON, LV_STATE_PRESSED);

        // IC number (left)
        lv_obj_t *lbl_num = lv_label_create(row);
        lv_label_set_text(lbl_num, ic->ic_number);
        lv_obj_set_style_text_font(lbl_num, FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(lbl_num, CLR_NEON, 0);
        lv_obj_align(lbl_num, LV_ALIGN_LEFT_MID, 4, 0);

        // Full name (centre)
        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, ic->full_name);
        lv_obj_set_style_text_font(lbl_name, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_name, CLR_TEXT_DIM, 0);
        lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(lbl_name, 140);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 60, 0);

        // Pin count badge (right)
        char badge[8];
        snprintf(badge, sizeof(badge), "%d-pin", ic->pin_count);
        lv_obj_t *lbl_pins = lv_label_create(row);
        lv_label_set_text(lbl_pins, badge);
        lv_obj_set_style_text_font(lbl_pins, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_pins, CLR_TEXT_LABEL, 0);
        lv_obj_align(lbl_pins, LV_ALIGN_RIGHT_MID, -4, 0);
    }

    // ── Test selected button ─────────────────────────────────────────────
    lv_obj_t *btn_test = lv_btn_create(scr_select);
    lv_obj_set_size(btn_test, DISPLAY_WIDTH - 24, 38);
    lv_obj_align(btn_test, LV_ALIGN_BOTTOM_MID, 0, -6);
    theme_apply_btn(btn_test);
    lv_obj_set_style_border_color(btn_test, CLR_SUCCESS, 0);
    lv_obj_set_style_shadow_color(btn_test, CLR_SUCCESS, 0);
    lv_obj_add_event_cb(btn_test, on_test_selected, LV_EVENT_ALL, nullptr);

    lv_obj_t *lbl_test = lv_label_create(btn_test);
    lv_label_set_text(lbl_test, LV_SYMBOL_PLAY "  TEST SELECTED IC");
    lv_obj_set_style_text_font(lbl_test, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_test, CLR_SUCCESS, 0);
    lv_obj_align(lbl_test, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load_anim(scr_select, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}

const ICDescriptor *ui_ic_select_get_selected(void) {
    return selected_ic;
}
