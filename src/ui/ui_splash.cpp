/**
 * ui_splash.cpp — Perfectly Centered Sci-Fi Holographic Radar & Oscilloscope Boot Screen
 *
 * Layout (320x240):
 *   - Top HUD (y=6..24): Compact, non-overlapping header with live pulsing LED dot & MCU tag.
 *   - Center Stage (Optical Center y=105):
 *       • 360° Holographic Arc Radar (114x114) with Crosshair Reticle & Concentric Guide Ring.
 *       • Inner Illuminated Core Badge (76x76) featuring bold "BAIUST" & "IC CHECKER".
 *       • Left Flank: Live animated "CLK" logic pulse waveform card.
 *       • Right Flank: Live animated "BUS" digital data stream card.
 *   - Bottom Diagnostic Dock (y=194..234): Slim floating mint card with neon progress bar & live logs.
 */

#include "ui_splash.h"
#include "ui_home.h"
#include "ui_theme.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <cstdio>

// ─── State ────────────────────────────────────────────────────────────────
static lv_obj_t *scr_splash        = nullptr;
static lv_obj_t *spinner_radar     = nullptr;
static lv_obj_t *core_circle       = nullptr;
static lv_obj_t *lbl_wave_clk      = nullptr;
static lv_obj_t *lbl_wave_data     = nullptr;
static lv_obj_t *bar_boot          = nullptr;
static lv_obj_t *lbl_status        = nullptr;
static lv_obj_t *lbl_percent       = nullptr;
static lv_obj_t *dot_radar         = nullptr;

static lv_timer_t *t_wave_anim     = nullptr;
static lv_timer_t *t_boot_seq      = nullptr;

static uint8_t  progress_val       = 0;
static uint16_t wave_tick          = 0;

// ─── Oscilloscope Waveform Frames ─────────────────────────────────────────
static const char *clk_frames[] = {
    "_ - _ - _",
    "- _ - _ -",
};

static const char *data_frames[] = {
    "--__--_",
    "_--__--",
    "__--__-",
    "-__--__",
};
constexpr uint8_t NUM_DATA_FRAMES = sizeof(data_frames) / sizeof(data_frames[0]);

// ─── Boot Diagnostic Logs ─────────────────────────────────────────────────
static const char *diag_logs[] = {
    ">> RADAR: INITIALIZING BUS...",
    ">> OSC: 240MHz CLOCK LOCKED",
    ">> MEM: 16MB FLASH + PSRAM OK",
    ">> ZIF-16: VOLTAGE STABILIZED",
    ">> SYSTEM ONLINE // BAIUST READY",
};
constexpr uint8_t NUM_DIAG_LOGS = sizeof(diag_logs) / sizeof(diag_logs[0]);

// ─── Live Waveform & Pulse Animation (fires every 80ms) ───────────────────
static void wave_anim_cb(lv_timer_t *t) {
    (void)t;
    if (!scr_splash) return;

    wave_tick++;

    // 1. Animate CLK wave
    if (lbl_wave_clk) {
        lv_label_set_text(lbl_wave_clk, clk_frames[wave_tick % 2]);
    }

    // 2. Animate BUS data stream
    if (lbl_wave_data) {
        lv_label_set_text(lbl_wave_data, data_frames[wave_tick % NUM_DATA_FRAMES]);
    }

    // 3. Pulse radar status indicator dot
    if (dot_radar) {
        bool on = (wave_tick % 2 == 0);
        lv_obj_set_style_bg_color(dot_radar, on ? CLR_NEON : CLR_TEXT_DIM, 0);
    }
}

// ─── Main Boot Sequencer (fires every 45ms) ───────────────────────────────
static void boot_sequence_cb(lv_timer_t *t) {
    (void)t;
    if (!scr_splash) return;

    progress_val += 2;

    if (progress_val <= 100) {
        // Update bar
        if (bar_boot) {
            lv_bar_set_value(bar_boot, progress_val, LV_ANIM_OFF);
        }

        // Update percentage label
        if (lbl_percent) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%d%%", progress_val);
            lv_label_set_text(lbl_percent, buf);
        }

        // Update diagnostic message
        if (lbl_status) {
            uint8_t msg_idx = (progress_val * (NUM_DIAG_LOGS - 1)) / 100;
            if (msg_idx >= NUM_DIAG_LOGS) msg_idx = NUM_DIAG_LOGS - 1;
            lv_label_set_text(lbl_status, diag_logs[msg_idx]);
        }
    } else {
        // Boot completed — stop timers
        if (t_wave_anim) {
            lv_timer_del(t_wave_anim);
            t_wave_anim = nullptr;
        }
        if (t_boot_seq) {
            lv_timer_del(t_boot_seq);
            t_boot_seq = nullptr;
        }

        // Ascending melodic power-on chime
        buzzer_hal_tone(880, 40);
        buzzer_hal_tone(1175, 50);
        buzzer_hal_tone(1568, 70);

        // Transition to Home Screen
        ui_home_show();
    }
}

// ─── Helper: Draw Cyberpunk HUD Corner Brackets ───────────────────────────
static void draw_hud_corner(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, bool left, bool top) {
    const lv_coord_t SZ = 12;
    const lv_coord_t TH = 2;

    // Horizontal line
    lv_obj_t *h = lv_obj_create(parent);
    lv_obj_set_size(h, SZ, TH);
    lv_obj_align(h, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(h, CLR_NEON, 0);
    lv_obj_set_style_bg_opa(h, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(h, 0, 0);

    // Vertical line
    lv_obj_t *v = lv_obj_create(parent);
    lv_obj_set_size(v, TH, SZ);
    lv_obj_align(v, align, left ? x_ofs : (x_ofs - TH + SZ), top ? y_ofs : (y_ofs - SZ + TH));
    lv_obj_set_style_bg_color(v, CLR_NEON, 0);
    lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(v, 0, 0);
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_splash_show(void) {
    progress_val = 0;
    wave_tick    = 0;

    // 1. Create screen with Solid Emerald Canvas
    scr_splash = lv_obj_create(nullptr);
    theme_apply_screen(scr_splash);
    lv_scr_load(scr_splash);

    // 2. Subtle Background Grid Lines
    for (int i = 1; i <= 5; i++) {
        lv_obj_t *line = lv_obj_create(scr_splash);
        lv_obj_set_size(line, DISPLAY_WIDTH, 1);
        lv_obj_set_pos(line, 0, i * 40);
        lv_obj_set_style_bg_color(line, CLR_GRID, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_30, 0);
        lv_obj_set_style_border_width(line, 0, 0);
    }

    // 3. Cyberpunk HUD Corner Brackets
    draw_hud_corner(scr_splash, LV_ALIGN_TOP_LEFT,     6,  6, true,  true);
    draw_hud_corner(scr_splash, LV_ALIGN_TOP_RIGHT,   -6,  6, false, true);
    draw_hud_corner(scr_splash, LV_ALIGN_BOTTOM_LEFT,  6, -6, true,  false);
    draw_hud_corner(scr_splash, LV_ALIGN_BOTTOM_RIGHT, -6, -6, false, false);

    // 4. Top HUD Header (Compact & No-Overlap Layout)
    lv_obj_t *hud_bar = lv_obj_create(scr_splash);
    lv_obj_set_size(hud_bar, DISPLAY_WIDTH - 24, 20);
    lv_obj_align(hud_bar, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_bg_opa(hud_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hud_bar, 0, 0);
    lv_obj_set_style_pad_all(hud_bar, 0, 0);
    lv_obj_clear_flag(hud_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Blinking status LED
    dot_radar = lv_obj_create(hud_bar);
    lv_obj_set_size(dot_radar, 8, 8);
    lv_obj_align(dot_radar, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(dot_radar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot_radar, CLR_NEON, 0);
    lv_obj_set_style_border_width(dot_radar, 0, 0);

    // Header Title (Left side)
    lv_obj_t *lbl_hud_title = lv_label_create(hud_bar);
    lv_label_set_text(lbl_hud_title, "BAIUST LAB");
    lv_obj_set_style_text_font(lbl_hud_title, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_hud_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_hud_title, LV_ALIGN_LEFT_MID, 18, 0);

    // Header Telemetry Badge (Right side)
    lv_obj_t *lbl_hud_telemetry = lv_label_create(hud_bar);
    lv_label_set_text(lbl_hud_telemetry, "ESP32-S3 [240MHz]");
    lv_obj_set_style_text_font(lbl_hud_telemetry, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_hud_telemetry, CLR_BLUE_DIM, 0);
    lv_obj_align(lbl_hud_telemetry, LV_ALIGN_RIGHT_MID, -4, 0);

    // Divider Line below HUD
    lv_obj_t *hud_div = lv_obj_create(scr_splash);
    lv_obj_set_size(hud_div, DISPLAY_WIDTH - 24, 1);
    lv_obj_align(hud_div, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_bg_color(hud_div, CLR_SEPARATOR, 0);
    lv_obj_set_style_bg_opa(hud_div, LV_OPA_40, 0);
    lv_obj_set_style_border_width(hud_div, 0, 0);

    // 5. Centerpiece: Optical Center Y = 106px (perfect vertical center between header and footer)
    const lv_coord_t CENTER_Y = 106;

    // Outer Crosshair Reticle Lines through Center
    lv_obj_t *ch_h = lv_obj_create(scr_splash);
    lv_obj_set_size(ch_h, 150, 1);
    lv_obj_set_pos(ch_h, (DISPLAY_WIDTH - 150) / 2, CENTER_Y);
    lv_obj_set_style_bg_color(ch_h, CLR_CYAN, 0);
    lv_obj_set_style_bg_opa(ch_h, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_h, 0, 0);

    lv_obj_t *ch_v = lv_obj_create(scr_splash);
    lv_obj_set_size(ch_v, 1, 130);
    lv_obj_set_pos(ch_v, DISPLAY_WIDTH / 2, CENTER_Y - 65);
    lv_obj_set_style_bg_color(ch_v, CLR_CYAN, 0);
    lv_obj_set_style_bg_opa(ch_v, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_v, 0, 0);

    // Concentric Guide Ring (Outer)
    lv_obj_t *guide_ring = lv_obj_create(scr_splash);
    lv_obj_set_size(guide_ring, 126, 126);
    lv_obj_set_pos(guide_ring, (DISPLAY_WIDTH - 126) / 2, CENTER_Y - 63);
    lv_obj_set_style_radius(guide_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(guide_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(guide_ring, CLR_CYAN, 0);
    lv_obj_set_style_border_width(guide_ring, 1, 0);
    lv_obj_set_style_border_opa(guide_ring, LV_OPA_50, 0);
    lv_obj_clear_flag(guide_ring, LV_OBJ_FLAG_CLICKABLE);

    // 360° Rotating Holographic Radar Spinner (faster 850ms period)
    spinner_radar = lv_spinner_create(scr_splash, 850, 75);
    lv_obj_set_size(spinner_radar, 114, 114);
    lv_obj_set_pos(spinner_radar, (DISPLAY_WIDTH - 114) / 2, CENTER_Y - 57);
    lv_obj_set_style_arc_color(spinner_radar, CLR_CYAN, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner_radar, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner_radar, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner_radar, 4, LV_PART_INDICATOR);
    lv_obj_clear_flag(spinner_radar, LV_OBJ_FLAG_CLICKABLE);

    // Center Illuminated Silicon Core Badge
    core_circle = lv_obj_create(scr_splash);
    lv_obj_set_size(core_circle, 78, 78);
    lv_obj_set_pos(core_circle, (DISPLAY_WIDTH - 78) / 2, CENTER_Y - 39);
    lv_obj_set_style_radius(core_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(core_circle, lv_color_hex(0x022C22), 0); // Dark silicon
    lv_obj_set_style_bg_opa(core_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(core_circle, CLR_NEON, 0);
    lv_obj_set_style_border_width(core_circle, 2, 0);
    lv_obj_set_style_shadow_color(core_circle, CLR_NEON, 0);
    lv_obj_set_style_shadow_width(core_circle, 16, 0);
    lv_obj_set_style_shadow_opa(core_circle, LV_OPA_60, 0);
    lv_obj_clear_flag(core_circle, LV_OBJ_FLAG_SCROLLABLE);

    // Core Brand Text
    lv_obj_t *lbl_brand = lv_label_create(core_circle);
    lv_label_set_text(lbl_brand, "BAIUST");
    lv_obj_set_style_text_font(lbl_brand, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_brand, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(lbl_brand, 1, 0);
    lv_obj_align(lbl_brand, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t *lbl_sub = lv_label_create(core_circle);
    lv_label_set_text(lbl_sub, "IC TESTER");
    lv_obj_set_style_text_font(lbl_sub, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_sub, CLR_NEON, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 10);

    // 6. Oscilloscope Waveform Flank Cards (Left & Right)
    // Left Flank (CLK Channel)
    lv_obj_t *card_clk = lv_obj_create(scr_splash);
    lv_obj_set_size(card_clk, 66, 42);
    lv_obj_set_pos(card_clk, 14, CENTER_Y - 21);
    lv_obj_set_style_bg_color(card_clk, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_bg_opa(card_clk, LV_OPA_80, 0);
    lv_obj_set_style_border_color(card_clk, CLR_CYAN, 0);
    lv_obj_set_style_border_width(card_clk, 1, 0);
    lv_obj_set_style_radius(card_clk, 6, 0);
    lv_obj_set_style_pad_all(card_clk, 2, 0);
    lv_obj_clear_flag(card_clk, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_clk_tag = lv_label_create(card_clk);
    lv_label_set_text(lbl_clk_tag, "CLK [CH1]");
    lv_obj_set_style_text_font(lbl_clk_tag, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_clk_tag, CLR_BLUE_DIM, 0);
    lv_obj_align(lbl_clk_tag, LV_ALIGN_TOP_MID, 0, 2);

    lbl_wave_clk = lv_label_create(card_clk);
    lv_label_set_text(lbl_wave_clk, clk_frames[0]);
    lv_obj_set_style_text_font(lbl_wave_clk, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_wave_clk, CLR_NEON, 0);
    lv_obj_align(lbl_wave_clk, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Right Flank (BUS Channel)
    lv_obj_t *card_data = lv_obj_create(scr_splash);
    lv_obj_set_size(card_data, 66, 42);
    lv_obj_set_pos(card_data, DISPLAY_WIDTH - 14 - 66, CENTER_Y - 21);
    lv_obj_set_style_bg_color(card_data, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_bg_opa(card_data, LV_OPA_80, 0);
    lv_obj_set_style_border_color(card_data, CLR_CYAN, 0);
    lv_obj_set_style_border_width(card_data, 1, 0);
    lv_obj_set_style_radius(card_data, 6, 0);
    lv_obj_set_style_pad_all(card_data, 2, 0);
    lv_obj_clear_flag(card_data, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_bus_tag = lv_label_create(card_data);
    lv_label_set_text(lbl_bus_tag, "BUS [CH2]");
    lv_obj_set_style_text_font(lbl_bus_tag, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_bus_tag, CLR_BLUE_DIM, 0);
    lv_obj_align(lbl_bus_tag, LV_ALIGN_TOP_MID, 0, 2);

    lbl_wave_data = lv_label_create(card_data);
    lv_label_set_text(lbl_wave_data, data_frames[0]);
    lv_obj_set_style_text_font(lbl_wave_data, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_wave_data, CLR_NEON, 0);
    lv_obj_align(lbl_wave_data, LV_ALIGN_BOTTOM_MID, 0, -2);

    // 7. Bottom Floating Diagnostic Card (y=182..234, Height=52px)
    lv_obj_t *diag_card = lv_obj_create(scr_splash);
    lv_obj_set_size(diag_card, DISPLAY_WIDTH - 24, 52);
    lv_obj_align(diag_card, LV_ALIGN_BOTTOM_MID, 0, -6);
    theme_apply_panel(diag_card);
    lv_obj_set_style_pad_all(diag_card, 4, 0);
    lv_obj_clear_flag(diag_card, LV_OBJ_FLAG_SCROLLABLE);

    // Top Row: Progress Bar + Percentage
    bar_boot = lv_bar_create(diag_card);
    lv_obj_set_size(bar_boot, 230, 6);
    lv_obj_align(bar_boot, LV_ALIGN_TOP_LEFT, 6, 4);
    lv_bar_set_range(bar_boot, 0, 100);
    lv_bar_set_value(bar_boot, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(bar_boot, CLR_BLUE_GLOW, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_boot, CLR_BLUE_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_boot, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_boot, 3, LV_PART_MAIN);

    lv_obj_set_style_bg_color(bar_boot, CLR_BG_DARK, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar_boot, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(bar_boot, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_boot, 3, LV_PART_INDICATOR);

    lbl_percent = lv_label_create(diag_card);
    lv_label_set_text(lbl_percent, "0%");
    lv_obj_set_style_text_font(lbl_percent, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_percent, CLR_TEXT, 0);
    lv_obj_align(lbl_percent, LV_ALIGN_TOP_RIGHT, -6, 1);

    // Bottom Row: Live Diagnostic Text
    lbl_status = lv_label_create(diag_card);
    lv_label_set_text(lbl_status, diag_logs[0]);
    lv_obj_set_style_text_font(lbl_status, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_status, CLR_TEXT_DIM, 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_LEFT, 6, -2);

    // 8. Start Animation Timers (30% faster boot sequence ~1.5s total)
    t_wave_anim = lv_timer_create(wave_anim_cb, 55, nullptr);
    t_boot_seq  = lv_timer_create(boot_sequence_cb, 30, nullptr);
}

void ui_splash_destroy(void) {
    if (t_wave_anim) {
        lv_timer_del(t_wave_anim);
        t_wave_anim = nullptr;
    }
    if (t_boot_seq) {
        lv_timer_del(t_boot_seq);
        t_boot_seq = nullptr;
    }
    if (scr_splash) {
        lv_obj_del(scr_splash);
        scr_splash = nullptr;
    }
}
