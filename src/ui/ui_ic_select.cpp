/**
 * ui_ic_select.cpp — Scrollable IC Browser (Deep Forest Emerald & Mint Theme)
 *
 * Layout (320x240 landscape):
 * ┌────────────────────────────────────────┐
 * │ [← BACK]       SELECT IC       [7408]  │ h=40 (White text / Mint buttons)
 * ├────────────────────────────────────────┤
 * │ ┌────────────────────────────────────┐ │
 * │ │ 7400  Quad NAND Gate       14-pin  │ │
 * │ │ 7402  Quad NOR Gate        14-pin  │ │ h=144 (Mint Floating Card List)
 * │ │ 7404  Hex Inverter         14-pin  │ │
 * │ │ 7408  Quad AND Gate        14-pin  │ │
 * │ └────────────────────────────────────┘ │
 * ├────────────────────────────────────────┤
 * │       [ ▶ TEST SELECTED IC ]           │ h=40 (Full-width Emerald Button)
 * └────────────────────────────────────────┘
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
    if (warn_box) return;
    
    buzzer_hal_beep_bad();
    
    warn_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(warn_box, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(warn_box, 0, 0);
    lv_obj_set_style_bg_color(warn_box, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_bg_opa(warn_box, LV_OPA_70, 0);
    lv_obj_set_style_border_width(warn_box, 0, 0);
    lv_obj_clear_flag(warn_box, LV_OBJ_FLAG_SCROLLABLE);
    
    // Warning card
    lv_obj_t *card = lv_obj_create(warn_box);
    lv_obj_set_size(card, 260, 126);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    theme_apply_panel(card);
    lv_obj_set_style_border_color(card, CLR_ERROR, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Warning icon + text
    lv_obj_t *lbl_warn = lv_label_create(card);
    lv_label_set_text(lbl_warn, LV_SYMBOL_WARNING "  NO IC DETECTED");
    lv_obj_set_style_text_font(lbl_warn, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_warn, CLR_ERROR, 0);
    lv_obj_align(lbl_warn, LV_ALIGN_TOP_MID, 0, 6);
    
    lv_obj_t *lbl_msg = lv_label_create(card);
    lv_label_set_text(lbl_msg, "Please insert IC into ZIF socket\nbefore starting test.");
    lv_obj_set_style_text_font(lbl_msg, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_msg, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, 4);
    
    // OK button
    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_size(btn_ok, 90, 30);
    lv_obj_align(btn_ok, LV_ALIGN_BOTTOM_MID, 0, -4);
    theme_apply_btn(btn_ok);
    lv_obj_add_event_cb(btn_ok, close_warning, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "GOT IT");
    lv_obj_set_style_text_font(lbl_ok, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_ok, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_ok, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_add_event_cb(warn_box, close_warning, LV_EVENT_ALL, nullptr);
}

static void on_test_selected(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!selected_ic) return;
    
    if (!zif_hal_detect_ic_presence()) {
        show_no_ic_warning();
        return;
    }
    
    ui_test_running_show(selected_ic, false);
}

void ui_ic_select_show(void) {
    if (!selected_ic && ic_db_count() > 0) {
        selected_ic = ic_db_get(0);
    }

    scr_select = lv_obj_create(nullptr);
    theme_apply_screen(scr_select);

    // ── TOP HEADER (White Title & Mint Controls) ──────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr_select);
    lv_obj_set_size(hdr, DISPLAY_WIDTH, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Back button
    lv_obj_t *btn_back = lv_btn_create(hdr);
    lv_obj_set_size(btn_back, 64, 28);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 12, 0);
    theme_apply_btn_secondary(btn_back);
    lv_obj_add_event_cb(btn_back, on_back, LV_EVENT_ALL, nullptr);
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_font(lbl_back, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_back, CLR_TEXT, 0);
    lv_obj_align(lbl_back, LV_ALIGN_CENTER, 0, 0);

    // Title in White
    lv_obj_t *lbl_hdr_title = lv_label_create(hdr);
    lv_label_set_text(lbl_hdr_title, "SELECT IC");
    lv_obj_set_style_text_font(lbl_hdr_title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_hdr_title, lv_color_hex(0xFFFFFF), 0); // Pure White on Canvas
    lv_obj_align(lbl_hdr_title, LV_ALIGN_CENTER, 0, 0);

    // Selected IC indicator badge
    lv_obj_t *badge_sel = lv_obj_create(hdr);
    lv_obj_set_size(badge_sel, 68, 26);
    lv_obj_align(badge_sel, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_bg_color(badge_sel, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(badge_sel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge_sel, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(badge_sel, 1, 0);
    lv_obj_set_style_radius(badge_sel, 13, 0);
    lv_obj_set_style_pad_all(badge_sel, 0, 0);
    lv_obj_clear_flag(badge_sel, LV_OBJ_FLAG_SCROLLABLE);

    lbl_selected = lv_label_create(badge_sel);
    lv_label_set_text(lbl_selected, selected_ic ? selected_ic->ic_number : "---");
    lv_obj_set_style_text_font(lbl_selected, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_selected, CLR_TEXT, 0);
    lv_obj_align(lbl_selected, LV_ALIGN_CENTER, 0, 0);

    // ── Scrollable IC list (Floating Mint Card) ──────────────────────────────
    lv_obj_t *list = lv_obj_create(scr_select);
    lv_obj_set_size(list, 296, 140);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 42);
    theme_apply_panel(list);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    uint8_t count = ic_db_count();
    for (uint8_t i = 0; i < count; i++) {
        const ICDescriptor *ic = ic_db_get(i);

        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), 38);
        lv_obj_set_style_bg_color(row, CLR_BLUE_GLOW, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, CLR_BLUE_DIM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row, on_ic_item_click, LV_EVENT_ALL, (void *)(uintptr_t)i);

        // Pressed state
        lv_obj_set_style_bg_color(row, CLR_BG_DARK, LV_STATE_PRESSED);

        // IC number (left)
        lv_obj_t *lbl_num = lv_label_create(row);
        lv_label_set_text(lbl_num, ic->ic_number);
        lv_obj_set_style_text_font(lbl_num, FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(lbl_num, CLR_TEXT_LABEL, 0); // Deep Forest Green
        lv_obj_align(lbl_num, LV_ALIGN_LEFT_MID, 6, 0);

        // Full name (centre)
        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, ic->full_name);
        lv_obj_set_style_text_font(lbl_name, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_name, CLR_TEXT_DIM, 0);
        lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(lbl_name, 140);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 68, 0);

        // Pin count badge (right)
        char badge[8];
        snprintf(badge, sizeof(badge), "%d-pin", ic->pin_count);
        lv_obj_t *lbl_pins = lv_label_create(row);
        lv_label_set_text(lbl_pins, badge);
        lv_obj_set_style_text_font(lbl_pins, FONT_TINY, 0);
        lv_obj_set_style_text_color(lbl_pins, CLR_TEXT, 0);
        lv_obj_align(lbl_pins, LV_ALIGN_RIGHT_MID, -6, 0);
    }

    // ── Full-width Emerald Test Button ───────────────────────────────────────
    lv_obj_t *btn_test = lv_btn_create(scr_select);
    lv_obj_set_size(btn_test, 296, 40);
    lv_obj_align(btn_test, LV_ALIGN_BOTTOM_MID, 0, -8);
    theme_apply_btn(btn_test);
    lv_obj_add_event_cb(btn_test, on_test_selected, LV_EVENT_ALL, nullptr);

    lv_obj_t *lbl_test = lv_label_create(btn_test);
    lv_label_set_text(lbl_test, LV_SYMBOL_PLAY "  TEST SELECTED IC");
    lv_obj_set_style_text_font(lbl_test, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_test, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_test, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load_anim(scr_select, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}

const ICDescriptor *ui_ic_select_get_selected(void) {
    return selected_ic;
}
