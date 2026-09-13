/**
 * ui_theme.h — Deep Forest Emerald & Mint Design System
 *
 * Rich, colorful theme featuring:
 * - Solid Deep Forest Emerald Canvas (#064E3B)
 * - Solid Soft Mint Green Floating Cards (#ECFDF5 / #D1FAE5) with crisp Mint borders (#A7F3D0)
 * - Vibrant Emerald Green Action Buttons (#059669 / #10B981)
 * - Crisp High-Contrast Deep Forest Slate Typography (#022C22 / #064E3B) & White (#FFFFFF) for headers
 */
#pragma once
#include <lvgl.h>

// ─── Color Palette ────────────────────────────────────────────────────────
#define CLR_BG          lv_color_hex(0x064E3B)   // Solid Deep Forest Emerald background
#define CLR_BG_PANEL    lv_color_hex(0xECFDF5)   // Solid Mint 50 for floating cards/panels
#define CLR_BG_DARK     lv_color_hex(0x059669)   // Emerald 600 - Primary actions & buttons

#define CLR_NEON        lv_color_hex(0x10B981)   // Emerald 500 - Active indicators & highlights
#define CLR_CYAN        lv_color_hex(0x047857)   // Emerald 700 - Deep emerald
#define CLR_BLUE_DIM    lv_color_hex(0xA7F3D0)   // Mint 200 - Clean crisp border for mint cards
#define CLR_BLUE_GLOW   lv_color_hex(0xD1FAE5)   // Mint 100 - Active card rows

#define CLR_SUCCESS     lv_color_hex(0x059669)   // Vibrant Emerald Green
#define CLR_SUCCESS_DIM lv_color_hex(0xD1FAE5)   // Mint 100 - Success badge background
#define CLR_ERROR       lv_color_hex(0xEF4444)   // Vibrant Rose Red
#define CLR_ERROR_DIM   lv_color_hex(0xFEE2E2)   // Rose 100 - Error badge background
#define CLR_WARNING     lv_color_hex(0xF59E0B)   // Warm Amber

#define CLR_TEXT        lv_color_hex(0x064E3B)   // Deep Forest Green - High contrast on Mint
#define CLR_TEXT_DIM    lv_color_hex(0x047857)   // Muted Emerald for sub-labels
#define CLR_TEXT_LABEL  lv_color_hex(0x022C22)   // Darkest Forest for headings

#define CLR_GRID        lv_color_hex(0x065F46)   // Subtle grid
#define CLR_SEPARATOR   lv_color_hex(0xA7F3D0)   // Mint divider lines

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
 * @brief Apply the solid deep forest emerald background to a screen object.
 */
static inline void theme_apply_screen(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, CLR_BG, 0);
    lv_obj_set_style_bg_opa(scr, OPA_FULL, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
}

/**
 * @brief Style a container as a solid mint floating card with subtle shadow and border.
 */
static inline void theme_apply_panel(lv_obj_t *panel) {
    lv_obj_set_style_bg_color(panel, CLR_BG_PANEL, 0); // Mint background
    lv_obj_set_style_bg_opa(panel, OPA_FULL, 0);
    lv_obj_set_style_border_color(panel, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 14, 0); // Smooth rounded corners
    
    // Soft drop shadow for floating effect
    lv_obj_set_style_shadow_color(panel, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_shadow_width(panel, 10, 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
}

/**
 * @brief Style a primary action button (Vibrant Emerald with white text).
 */
static inline void theme_apply_btn(lv_obj_t *btn) {
    lv_obj_set_style_bg_color(btn, CLR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(btn, OPA_FULL, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    
    // Soft shadow
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_shadow_width(btn, 8, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);

    // Pressed state
    lv_obj_set_style_bg_color(btn, CLR_CYAN, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, LV_STATE_PRESSED);
}

/**
 * @brief Style a secondary button (Mint card button with deep forest green text).
 */
static inline void theme_apply_btn_secondary(lv_obj_t *btn) {
    lv_obj_set_style_bg_color(btn, CLR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, OPA_FULL, 0);
    lv_obj_set_style_border_color(btn, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_text_color(btn, CLR_TEXT, 0);

    lv_obj_set_style_bg_color(btn, CLR_BLUE_GLOW, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, CLR_NEON, LV_STATE_PRESSED);
}

/**
 * @brief Apply primary emerald text label style.
 */
static inline void theme_apply_label_neon(lv_obj_t *lbl) {
    lv_obj_set_style_text_color(lbl, CLR_BG_DARK, 0);
    lv_obj_set_style_text_opa(lbl, OPA_FULL, 0);
}

/**
 * @brief Style a floating navigation pill.
 */
static inline void theme_apply_nav_pill(lv_obj_t *nav) {
    lv_obj_set_style_bg_color(nav, CLR_BG_PANEL, 0); // Mint pill
    lv_obj_set_style_bg_opa(nav, OPA_FULL, 0);
    lv_obj_set_style_radius(nav, 28, 0); // Smooth pill shape
    lv_obj_set_style_border_color(nav, CLR_BLUE_DIM, 0);
    lv_obj_set_style_border_width(nav, 1, 0);
    lv_obj_set_style_pad_all(nav, 0, 0);
    
    // Soft floating shadow
    lv_obj_set_style_shadow_color(nav, lv_color_hex(0x022C22), 0);
    lv_obj_set_style_shadow_width(nav, 12, 0);
    lv_obj_set_style_shadow_opa(nav, LV_OPA_30, 0);
}

/**
 * @brief Apply SUCCESS style to an object.
 */
static inline void theme_apply_success(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_SUCCESS_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_FULL, 0);
    lv_obj_set_style_border_color(obj, CLR_SUCCESS, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_text_color(obj, CLR_BG_DARK, 0);
}

/**
 * @brief Apply ERROR style to an object.
 */
static inline void theme_apply_error(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, CLR_ERROR_DIM, 0);
    lv_obj_set_style_bg_opa(obj, OPA_FULL, 0);
    lv_obj_set_style_border_color(obj, CLR_ERROR, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_text_color(obj, CLR_ERROR, 0);
}
