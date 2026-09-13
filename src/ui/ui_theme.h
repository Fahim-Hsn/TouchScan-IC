/**
 * ui_theme.h — Sci-Fi / Holographic Design System
 *
 * Color palette: deep black + blue neon + holographic cyan
 * All UI colors and reusable style helpers defined here.
 */
#pragma once
#include <lvgl.h>

// ─── Color Palette ────────────────────────────────────────────────────────
// Light Blue & Light Green Modern Theme (Based on User Mockup)
#define CLR_BG          lv_color_hex(0xF4F6F9)   // Pristine light gray/off-white background
#define CLR_BG_PANEL    lv_color_hex(0xFFFFFF)   // Pure white for cards/panels
#define CLR_BG_DARK     lv_color_hex(0x0EA5E9)   // Sky Blue (Primary for headers / main cards)

#define CLR_NEON        lv_color_hex(0x38BDF8)   // Lighter Sky Blue for accents/pressed states
#define CLR_CYAN        lv_color_hex(0x10B981)   // Emerald Light Green (Nav pill / accents)
#define CLR_BLUE_DIM    lv_color_hex(0xE2E8F0)   // Light gray for borders
#define CLR_BLUE_GLOW   lv_color_hex(0x0284C7)   // Darker Sky Blue

#define CLR_SUCCESS     lv_color_hex(0x10B981)   // Emerald Green
#define CLR_SUCCESS_DIM lv_color_hex(0xD1FAE5)   // Light green background
#define CLR_ERROR       lv_color_hex(0xEF4444)   // Vibrant Red
#define CLR_ERROR_DIM   lv_color_hex(0xFEE2E2)   // Light red background
#define CLR_WARNING     lv_color_hex(0xF59E0B)   // Yellow

#define CLR_TEXT        lv_color_hex(0x1E293B)   // Dark Slate for readability on light bg
#define CLR_TEXT_DIM    lv_color_hex(0x94A3B8)   // Muted gray text
#define CLR_TEXT_LABEL  lv_color_hex(0x0F172A)   // Darkest text for headings

#define CLR_GRID        lv_color_hex(0xE5E7EB)   // Light gray grid
#define CLR_SEPARATOR   lv_color_hex(0xD1D5DB)   // Divider lines

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
 * @brief Apply the base light background to a screen object.
 */
static inline void theme_apply_screen(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, CLR_BG, 0);
    lv_obj_set_style_bg_opa(scr, OPA_FULL, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
}

/**
 * @brief Style a container as a clean white card with soft shadow.
 */
static inline void theme_apply_panel(lv_obj_t *panel) {
    lv_obj_set_style_bg_color(panel, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, OPA_FULL, 0);
    lv_obj_set_style_border_color(panel, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 12, 0); // Smooth rounded corners
    
    // Soft drop shadow
    lv_obj_set_style_shadow_color(panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(panel, 10, 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
}

/**
 * @brief Style a primary button (Deep Indigo with white text).
 */
static inline void theme_apply_btn(lv_obj_t *btn) {
    // Normal state
    lv_obj_set_style_bg_color(btn, CLR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(btn, OPA_FULL, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    
    // Soft shadow for depth
    lv_obj_set_style_shadow_color(btn, CLR_BG_DARK, 0);
    lv_obj_set_style_shadow_width(btn, 10, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0); // Force white text on dark button

    // Pressed state
    lv_obj_set_style_bg_color(btn, CLR_NEON, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, LV_STATE_PRESSED);
}

/**
 * @brief Apply primary indigo text label style.
 */
static inline void theme_apply_label_neon(lv_obj_t *lbl) {
    lv_obj_set_style_text_color(lbl, CLR_BG_DARK, 0); // Indigo text for emphasis
    lv_obj_set_style_text_opa(lbl, OPA_FULL, 0);
}

/**
 * @brief Style a floating navigation pill.
 */
static inline void theme_apply_nav_pill(lv_obj_t *nav) {
    lv_obj_set_style_bg_color(nav, CLR_CYAN, 0); // Light Green pill
    lv_obj_set_style_bg_opa(nav, OPA_FULL, 0);
    lv_obj_set_style_radius(nav, 26, 0); // Pill shape
    lv_obj_set_style_border_width(nav, 0, 0);
    lv_obj_set_style_pad_all(nav, 0, 0);
    
    // Soft drop shadow
    lv_obj_set_style_shadow_color(nav, CLR_CYAN, 0);
    lv_obj_set_style_shadow_width(nav, 15, 0);
    lv_obj_set_style_shadow_opa(nav, LV_OPA_40, 0);
}

/**
 * @brief Apply SUCCESS (green) style to an object.
 */
static inline void theme_apply_success(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_SUCCESS_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_FULL, 0);
    lv_obj_set_style_border_color(obj, CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_text_color(obj, CLR_SUCCESS, 0);
}

/**
 * @brief Apply ERROR (red) style to an object.
 */
static inline void theme_apply_error(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_FULL, 0);
    lv_obj_set_style_border_color(obj, CLR_ERROR, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_text_color(obj, CLR_ERROR, 0);
}

