/**
 * lv_conf.h — LVGL 8.3 Configuration
 * Digital IC Checker | ESP32-S3 N16R8
 * Optimized for Sci-Fi UI on 320x240 ST7789 display with PSRAM
 */
#if 1  /* Set to "1" to enable content */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH     16   /* RGB565 for ST7789 */
#define LV_COLOR_16_SWAP   0
#define LV_COLOR_SCREEN_TRANSP 0

/*====================
   MEMORY SETTINGS
 *====================*/
/* LVGL internal heap (objects, styles, animations) */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE   (128 * 1024U)  /* 128 KB — ESP32-S3 has enough DRAM */
#define LV_MEM_ADR    0              /* 0 = auto (malloc) */
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC   malloc
#define LV_MEM_POOL_FREE    free

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD  10   /* Default refresh period (ms) */
#define LV_INDEV_DEF_READ_PERIOD 30   /* Input device read period (ms) */

/*====================
   FEATURE SETTINGS
 *====================*/
/* Animations */
#define LV_USE_ANIMATION 1

/* Asserts (disable in production) */
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Printf-style log */
#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF       1
#define LV_LOG_TRACE_MEM    0
#define LV_LOG_TRACE_TIMER  0
#define LV_LOG_TRACE_INDEV  0
#define LV_LOG_TRACE_DISP   0
#define LV_LOG_TRACE_EVENT  0
#define LV_LOG_TRACE_OBJ_CREATE 0
#define LV_LOG_TRACE_LAYOUT 0
#define LV_LOG_TRACE_ANIM   0

/*===================
 *  DRAWING
 *==================*/
#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0
#define LV_CIRCLE_CACHE_SIZE 4
#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DITHER_GRADIENT 0
#define LV_DISP_ROT_MAX_BUF (10*1024)

/*===================
 *  GPU
 *==================*/
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_GPU_NXP_PXP     0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0
#define LV_USE_GPU_ESP_PPA      0  /* ESP32-S3 PPA not yet stable in LVGL 8.3 */

/*===================
 *  FONT USAGE
 *==================*/
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  1
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  1

/* Built-in fonts */
#define LV_FONT_UNSCII_8       0
#define LV_FONT_UNSCII_16      0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Symbols */
#define LV_USE_FONT_SUBPX  1
#define LV_FONT_SUBPX_BGR  0

/* Text settings */
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " "
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*===================
 *  WIDGET USAGE
 *==================*/
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1
#define LV_USE_TEXTAREA     1
#define LV_USE_CHART        0
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_KEYBOARD     0
#define LV_USE_LED          1
#define LV_USE_METER        0
#define LV_USE_MSGBOX       1
#define LV_USE_OBJMASK      0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0
#define LV_USE_SPAN         0
#define LV_USE_LIST         1

/* Table settings */
#define LV_TABLE_COLUMN_MAX 10

/* Label settings */
#define LV_LABEL_TEXT_SELECTION 1
#define LV_LABEL_LONG_TXT_HINT  1

/*===================
 *  THEME USAGE
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1    /* Dark mode by default */
#define LV_USE_THEME_BASIC   1
#define LV_USE_THEME_MONO    0

/*===================
 *  LAYOUTS
 *==================*/
#define LV_USE_FLEX  1
#define LV_USE_GRID  1

/*===================
 *  3rd PARTY
 *==================*/
#define LV_USE_FS_STDIO      0
#define LV_USE_FS_POSIX      0
#define LV_USE_FS_WIN32      0
#define LV_USE_FS_FATFS      0
#define LV_USE_PNG           0
#define LV_USE_BMP           0
#define LV_USE_SJPG          0
#define LV_USE_GIF           0
#define LV_USE_QRCODE        0
#define LV_USE_FREETYPE      0
#define LV_USE_RLOTTIE       0
#define LV_USE_FFMPEG        0

/*===================
 *  OTHERS
 *==================*/
#define LV_USE_SNAPSHOT    0
#define LV_USE_MONKEY      0
#define LV_USE_GRIDNAV     0
#define LV_USE_FRAGMENT    0
#define LV_USE_IMGFONT     0
#define LV_USE_MSG         0
#define LV_USE_IME_PINYIN  0

/* Profiler */
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_REFR_DEBUG       0

/* Demo */
#define LV_BUILD_EXAMPLES  0
#define LV_USE_DEMO_WIDGETS 0

#endif /*LV_CONF_H*/
#endif /*End of "Content enable"*/
