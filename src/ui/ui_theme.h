/**
 * ui_theme.h — Sci-Fi / Holographic Design System
 *
 * Color palette: deep black + blue neon + holographic cyan
 * All UI colors and reusable style helpers defined here.
 */
#pragma once
#include <lvgl.h>

// ─── Color Palette ────────────────────────────────────────────────────────
#define CLR_BG          lv_color_hex(0x000814)   // Deep space black
#define CLR_BG_PANEL    lv_color_hex(0x001428)   // Panel / card background
#define CLR_BG_DARK     lv_color_hex(0x000509)   // Darker background

#define CLR_NEON        lv_color_hex(0x00D4FF)   // Primary neon blue
#define CLR_CYAN        lv_color_hex(0x00FFFF)   // Bright cyan accent
#define CLR_BLUE_DIM    lv_color_hex(0x0055AA)   // Dimmer blue (borders)
#define CLR_BLUE_GLOW   lv_color_hex(0x003366)   // Glow/shadow colour

#define CLR_SUCCESS     lv_color_hex(0x00FF88)   // PASS green
#define CLR_SUCCESS_DIM lv_color_hex(0x007744)
#define CLR_ERROR       lv_color_hex(0xFF1144)   // FAIL red
#define CLR_ERROR_DIM   lv_color_hex(0x880022)
#define CLR_WARNING     lv_color_hex(0xFFAA00)   // Warning amber

#define CLR_TEXT        lv_color_hex(0xD0EEFF)   // Primary text
#define CLR_TEXT_DIM    lv_color_hex(0x6699BB)   // Secondary text
#define CLR_TEXT_LABEL  lv_color_hex(0x4488AA)   // Label/hint text

#define CLR_GRID        lv_color_hex(0x001E3A)   // Subtle grid lines
#define CLR_SEPARATOR   lv_color_hex(0x003355)   // Divider lines

// ─── Opacity Macros ───────────────────────────────────────────────────────
#define OPA_FULL    LV_OPA_COVER
#define OPA_HIGH    LV_OPA_90
#define OPA_MED     LV_OPA_60
#define OPA_LOW     LV_OPA_30
#define OPA_NONE    LV_OPA_TRANSP

// ─── Font Shortcuts ───────────────────────────────────────────────────────
#define FONT_TINY    &lv_font_montserrat_12
#define FONT_SMALL   &lv_font_montserrat_14
#define FONT_NORMAL  &lv_font_montserrat_16
#define FONT_MEDIUM  &lv_font_montserrat_20
#define FONT_LARGE   &lv_font_montserrat_24
#define FONT_XLARGE  &lv_font_montserrat_28
#define FONT_TITLE   &lv_font_montserrat_36
#define FONT_HERO    &lv_font_montserrat_48

// ─── Style Helpers ────────────────────────────────────────────────────────

/**
 * @brief Apply the base Sci-Fi dark background to a screen object.
 */
static inline void theme_apply_screen(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, CLR_BG, 0);
    lv_obj_set_style_bg_opa(scr, OPA_FULL, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
}

/**
 * @brief Style a container as a neon-bordered panel.
 */
static inline void theme_apply_panel(lv_obj_t *panel) {
    lv_obj_set_style_bg_color(panel, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, OPA_FULL, 0);
    lv_obj_set_style_border_color(panel, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
}

/**
 * @brief Style a button with Sci-Fi neon border + hover glow.
 */
static inline void theme_apply_btn(lv_obj_t *btn) {
    // Normal state
    lv_obj_set_style_bg_color(btn, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, OPA_FULL, 0);
    lv_obj_set_style_border_color(btn, CLR_NEON, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_shadow_color(btn, CLR_NEON, 0);
    lv_obj_set_style_shadow_width(btn, 8, 0);
    lv_obj_set_style_shadow_opa(btn, OPA_LOW, 0);

    // Pressed state
    lv_obj_set_style_bg_color(btn, CLR_BLUE_GLOW, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, CLR_CYAN, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn, OPA_HIGH, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 16, LV_STATE_PRESSED);
}

/**
 * @brief Apply neon-text label style.
 */
static inline void theme_apply_label_neon(lv_obj_t *lbl) {
    lv_obj_set_style_text_color(lbl, CLR_NEON, 0);
    lv_obj_set_style_text_opa(lbl, OPA_FULL, 0);
}

/**
 * @brief Apply SUCCESS (green) style to an object.
 */
static inline void theme_apply_success(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_SUCCESS_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_MED, 0);
    lv_obj_set_style_border_color(obj, CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
}

/**
 * @brief Apply ERROR (red) style to an object.
 */
static inline void theme_apply_error(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_MED, 0);
    lv_obj_set_style_border_color(obj, CLR_ERROR, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
}
