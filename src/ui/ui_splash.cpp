/**
 * ui_splash.cpp — High-Tech Sci-Fi "BAIUST IC CHECKER" Boot Screen
 *
 * Visual Features:
 *   - Futuristic HUD topbar with blinking status dot and system version.
 *   - Animated DIP-16 IC Chip graphic:
 *       • Silicon body with Pin 1 notch and "BAIUST / IC CHECKER" laser marking
 *       • 16 metallic DIP pins with sequential logic-pulse light scan
 *   - High-contrast diagnostics card with real-time percentage counter & live boot log:
 *       • [OK] ESP32-S3 Dual-Core @ 240MHz
 *       • [OK] 16MB Flash & 8MB PSRAM OK
 *       • [OK] 74xx / 40xx Logic Core Ready
 *       • [OK] ZIF-16 Socket Bus Active
 *       • [OK] BAIUST System Ready!
 *   - Melodic ascending boot sound & clean transition to Home screen.
 */

#include "ui_splash.h"
#include "ui_home.h"
#include "ui_theme.h"
#include "../hal/buzzer_hal.h"
#include "../config.h"
#include <lvgl.h>
#include <cstdio>

// ─── State ────────────────────────────────────────────────────────────────
static lv_obj_t *scr_splash      = nullptr;
static lv_obj_t *chip_body       = nullptr;
static lv_obj_t *lbl_chip_brand  = nullptr;
static lv_obj_t *lbl_chip_sub    = nullptr;
static lv_obj_t *bar_boot        = nullptr;
static lv_obj_t *lbl_status      = nullptr;
static lv_obj_t *lbl_percent     = nullptr;
static lv_obj_t *dot_status      = nullptr;

// 16 Pins for DIP-16 IC
static lv_obj_t *pins_top[8]     = {nullptr};
static lv_obj_t *pins_bot[8]     = {nullptr};

static lv_timer_t *t_boot_seq    = nullptr;
static lv_timer_t *t_pin_anim    = nullptr;
static uint8_t boot_progress     = 0;
static uint8_t active_pin_idx    = 0;

// ─── Boot Diagnostic Messages ─────────────────────────────────────────────
static const char *boot_msgs[] = {
    "BOOT: ESP32-S3 Core @ 240MHz",
    "MEM: 16MB Flash + PSRAM OK",
    "CORE: 74xx / 40xx Logic Loaded",
    "BUS: ZIF-16 Socket Active",
    "SYSTEM: BAIUST Ready!",
};
constexpr uint8_t NUM_BOOT_MSGS = sizeof(boot_msgs) / sizeof(boot_msgs[0]);

// ─── Pin Wave Scanning Animation ──────────────────────────────────────────
static void pin_scan_anim_cb(lv_timer_t *t) {
    (void)t;
    if (!scr_splash) return;

    // Reset previous pins to default inactive color
    for (int i = 0; i < 8; i++) {
        if (pins_top[i]) {
            lv_obj_set_style_bg_color(pins_top[i], CLR_BLUE_DIM, 0);
            lv_obj_set_style_shadow_width(pins_top[i], 0, 0);
        }
        if (pins_bot[i]) {
            lv_obj_set_style_bg_color(pins_bot[i], CLR_BLUE_DIM, 0);
            lv_obj_set_style_shadow_width(pins_bot[i], 0, 0);
        }
    }

    // Highlight current active pin with bright neon green
    uint8_t current_top = active_pin_idx;
    uint8_t current_bot = 7 - active_pin_idx; // Reverse scan on bottom to simulate loop

    if (pins_top[current_top]) {
        lv_obj_set_style_bg_color(pins_top[current_top], CLR_NEON, 0);
        lv_obj_set_style_shadow_color(pins_top[current_top], CLR_NEON, 0);
        lv_obj_set_style_shadow_width(pins_top[current_top], 6, 0);
        lv_obj_set_style_shadow_opa(pins_top[current_top], LV_OPA_COVER, 0);
    }
    if (pins_bot[current_bot]) {
        lv_obj_set_style_bg_color(pins_bot[current_bot], CLR_NEON, 0);
        lv_obj_set_style_shadow_color(pins_bot[current_bot], CLR_NEON, 0);
        lv_obj_set_style_shadow_width(pins_bot[current_bot], 6, 0);
        lv_obj_set_style_shadow_opa(pins_bot[current_bot], LV_OPA_COVER, 0);
    }

    active_pin_idx = (active_pin_idx + 1) % 8;

    // Blinking status dot in HUD
    if (dot_status) {
        bool on = (active_pin_idx % 2 == 0);
        lv_obj_set_style_bg_color(dot_status, on ? CLR_NEON : CLR_TEXT_DIM, 0);
    }
}

// ─── Main Boot Sequence Timer (runs every ~35ms) ──────────────────────────
static void boot_sequence_cb(lv_timer_t *t) {
    (void)t;
    if (!scr_splash) return;

    boot_progress += 2;

    if (boot_progress <= 100) {
        // Update bar
        if (bar_boot) {
            lv_bar_set_value(bar_boot, boot_progress, LV_ANIM_OFF);
        }

        // Update percentage
        if (lbl_percent) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%d%%", boot_progress);
            lv_label_set_text(lbl_percent, buf);
        }

        // Update log text based on progress
        if (lbl_status) {
            uint8_t msg_idx = (boot_progress * (NUM_BOOT_MSGS - 1)) / 100;
            if (msg_idx >= NUM_BOOT_MSGS) msg_idx = NUM_BOOT_MSGS - 1;
            lv_label_set_text(lbl_status, boot_msgs[msg_idx]);
        }
    } else {
        // Boot completed — stop timers
        if (t_boot_seq) {
            lv_timer_del(t_boot_seq);
            t_boot_seq = nullptr;
        }
        if (t_pin_anim) {
            lv_timer_del(t_pin_anim);
            t_pin_anim = nullptr;
        }

        // Melodic boot chirp
        buzzer_hal_tone(1046, 40); // C6
        buzzer_hal_tone(1318, 60); // E6

        // Transition to Home Screen
        ui_home_show();
    }
}

// ─── Public API ───────────────────────────────────────────────────────────
void ui_splash_show(void) {
    boot_progress  = 0;
    active_pin_idx = 0;

    // 1. Create screen with Solid Emerald Canvas
    scr_splash = lv_obj_create(nullptr);
    theme_apply_screen(scr_splash);
    lv_scr_load(scr_splash);

    // 2. Subtle Circuit Grid Lines in Background
    for (int i = 1; i <= 5; i++) {
        lv_obj_t *line = lv_obj_create(scr_splash);
        lv_obj_set_size(line, DISPLAY_WIDTH, 1);
        lv_obj_set_pos(line, 0, i * 40);
        lv_obj_set_style_bg_color(line, CLR_GRID, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_30, 0);
        lv_obj_set_style_border_width(line, 0, 0);
    }

    // 3. Futuristic HUD Top Bar
    lv_obj_t *hud_cont = lv_obj_create(scr_splash);
    lv_obj_set_size(hud_cont, DISPLAY_WIDTH - 20, 24);
    lv_obj_align(hud_cont, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_opa(hud_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hud_cont, 0, 0);
    lv_obj_set_style_pad_all(hud_cont, 0, 0);
    lv_obj_clear_flag(hud_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Blinking LED indicator
    dot_status = lv_obj_create(hud_cont);
    lv_obj_set_size(dot_status, 8, 8);
    lv_obj_align(dot_status, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(dot_status, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot_status, CLR_NEON, 0);
    lv_obj_set_style_border_width(dot_status, 0, 0);

    // HUD Title
    lv_obj_t *lbl_hud_title = lv_label_create(hud_cont);
    lv_label_set_text(lbl_hud_title, "BAIUST  ::  SYSTEM BOOT");
    lv_obj_set_style_text_font(lbl_hud_title, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_hud_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_hud_title, LV_ALIGN_LEFT_MID, 18, 0);

    // HUD Version / Badge
    lv_obj_t *lbl_hud_ver = lv_label_create(hud_cont);
    lv_label_set_text(lbl_hud_ver, "ESP32-S3 [v1.0]");
    lv_obj_set_style_text_font(lbl_hud_ver, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_hud_ver, CLR_BLUE_DIM, 0);
    lv_obj_align(lbl_hud_ver, LV_ALIGN_RIGHT_MID, -4, 0);

    // Divider Line below HUD
    lv_obj_t *hud_div = lv_obj_create(scr_splash);
    lv_obj_set_size(hud_div, DISPLAY_WIDTH - 24, 1);
    lv_obj_align(hud_div, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(hud_div, CLR_SEPARATOR, 0);
    lv_obj_set_style_bg_opa(hud_div, LV_OPA_50, 0);
    lv_obj_set_style_border_width(hud_div, 0, 0);

    // 4. Center High-Tech DIP-16 IC Graphic
    const lv_coord_t CHIP_W = 168;
    const lv_coord_t CHIP_H = 62;
    const lv_coord_t CHIP_CY = 96;

    // IC Metallic Pins (8 Top & 8 Bottom)
    const lv_coord_t PIN_W = 8;
    const lv_coord_t PIN_H = 8;
    const lv_coord_t PIN_SPACING = 18;
    const lv_coord_t PIN_START_X = -((7 * PIN_SPACING) / 2);

    for (int i = 0; i < 8; i++) {
        lv_coord_t px = PIN_START_X + (i * PIN_SPACING);

        // Top Pin
        pins_top[i] = lv_obj_create(scr_splash);
        lv_obj_set_size(pins_top[i], PIN_W, PIN_H);
        lv_obj_align(pins_top[i], LV_ALIGN_CENTER, px, CHIP_CY - 33);
        lv_obj_set_style_bg_color(pins_top[i], CLR_BLUE_DIM, 0);
        lv_obj_set_style_radius(pins_top[i], 2, 0);
        lv_obj_set_style_border_width(pins_top[i], 0, 0);

        // Bottom Pin
        pins_bot[i] = lv_obj_create(scr_splash);
        lv_obj_set_size(pins_bot[i], PIN_W, PIN_H);
        lv_obj_align(pins_bot[i], LV_ALIGN_CENTER, px, CHIP_CY + 33);
        lv_obj_set_style_bg_color(pins_bot[i], CLR_BLUE_DIM, 0);
        lv_obj_set_style_radius(pins_bot[i], 2, 0);
        lv_obj_set_style_border_width(pins_bot[i], 0, 0);
    }

    // IC Silicon Body
    chip_body = lv_obj_create(scr_splash);
    lv_obj_set_size(chip_body, CHIP_W, CHIP_H);
    lv_obj_align(chip_body, LV_ALIGN_CENTER, 0, CHIP_CY);
    lv_obj_set_style_bg_color(chip_body, lv_color_hex(0x022C22), 0); // Dark silicon
    lv_obj_set_style_bg_opa(chip_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chip_body, CLR_NEON, 0);
    lv_obj_set_style_border_width(chip_body, 2, 0);
    lv_obj_set_style_radius(chip_body, 8, 0);
    lv_obj_set_style_shadow_color(chip_body, CLR_NEON, 0);
    lv_obj_set_style_shadow_width(chip_body, 14, 0);
    lv_obj_set_style_shadow_opa(chip_body, LV_OPA_40, 0);
    lv_obj_clear_flag(chip_body, LV_OBJ_FLAG_SCROLLABLE);

    // Pin 1 Notch on Left Edge of Chip
    lv_obj_t *chip_notch = lv_obj_create(chip_body);
    lv_obj_set_size(chip_notch, 8, 14);
    lv_obj_align(chip_notch, LV_ALIGN_LEFT_MID, -6, 0);
    lv_obj_set_style_bg_color(chip_notch, CLR_BG, 0);
    lv_obj_set_style_border_color(chip_notch, CLR_NEON, 0);
    lv_obj_set_style_border_width(chip_notch, 1, 0);
    lv_obj_set_style_radius(chip_notch, 4, 0);

    // Laser-Engraved Branding Text on Chip
    lbl_chip_brand = lv_label_create(chip_body);
    lv_label_set_text(lbl_chip_brand, "BAIUST");
    lv_obj_set_style_text_font(lbl_chip_brand, FONT_LARGE, 0);
    lv_obj_set_style_text_color(lbl_chip_brand, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(lbl_chip_brand, 3, 0);
    lv_obj_align(lbl_chip_brand, LV_ALIGN_CENTER, 4, -9);

    lbl_chip_sub = lv_label_create(chip_body);
    lv_label_set_text(lbl_chip_sub, "IC CHECKER");
    lv_obj_set_style_text_font(lbl_chip_sub, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_chip_sub, CLR_NEON, 0);
    lv_obj_set_style_text_letter_space(lbl_chip_sub, 2, 0);
    lv_obj_align(lbl_chip_sub, LV_ALIGN_CENTER, 4, 13);

    // 5. Floating Mint Diagnostic Card at Bottom
    lv_obj_t *diag_card = lv_obj_create(scr_splash);
    lv_obj_set_size(diag_card, DISPLAY_WIDTH - 24, 66);
    lv_obj_align(diag_card, LV_ALIGN_BOTTOM_MID, 0, -8);
    theme_apply_panel(diag_card);
    lv_obj_set_style_pad_all(diag_card, 6, 0);
    lv_obj_clear_flag(diag_card, LV_OBJ_FLAG_SCROLLABLE);

    // Progress Bar + Percentage Row
    bar_boot = lv_bar_create(diag_card);
    lv_obj_set_size(bar_boot, 230, 8);
    lv_obj_align(bar_boot, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_bar_set_range(bar_boot, 0, 100);
    lv_bar_set_value(bar_boot, 0, LV_ANIM_OFF);

    // Bar Styling
    lv_obj_set_style_bg_color(bar_boot, CLR_BLUE_GLOW, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_boot, CLR_BLUE_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_boot, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_boot, 4, LV_PART_MAIN);

    lv_obj_set_style_bg_color(bar_boot, CLR_BG_DARK, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar_boot, CLR_NEON, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(bar_boot, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_boot, 4, LV_PART_INDICATOR);

    // Percentage Label
    lbl_percent = lv_label_create(diag_card);
    lv_label_set_text(lbl_percent, "0%");
    lv_obj_set_style_text_font(lbl_percent, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_percent, CLR_TEXT, 0);
    lv_obj_align(lbl_percent, LV_ALIGN_TOP_RIGHT, -6, 4);

    // Diagnostic Terminal Message Log
    lbl_status = lv_label_create(diag_card);
    lv_label_set_text(lbl_status, boot_msgs[0]);
    lv_obj_set_style_text_font(lbl_status, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl_status, CLR_TEXT_DIM, 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_LEFT, 6, -4);

    // 6. Start Animated Timers
    // Pin scanning runs every 120ms
    t_pin_anim = lv_timer_create(pin_scan_anim_cb, 120, nullptr);

    // Boot progress advances every ~45ms (~2.2s total smooth boot)
    t_boot_seq = lv_timer_create(boot_sequence_cb, 45, nullptr);
}

void ui_splash_destroy(void) {
    if (t_boot_seq) {
        lv_timer_del(t_boot_seq);
        t_boot_seq = nullptr;
    }
    if (t_pin_anim) {
        lv_timer_del(t_pin_anim);
        t_pin_anim = nullptr;
    }
    if (scr_splash) {
        lv_obj_del(scr_splash);
        scr_splash = nullptr;
    }
}
