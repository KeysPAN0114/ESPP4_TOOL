/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"

static lv_style_t style_bullet;
static lv_obj_t *scale1;
static const lv_font_t *font_normal = &lv_font_montserrat_16;

void example_lvgl_demo_ui(lv_display_t *disp)
{
    // init default theme
    lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
    // bullet style
    lv_style_init(&style_bullet);
    lv_style_set_border_width(&style_bullet, 0);
    lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

    lv_obj_t *screen = lv_display_get_screen_active(disp);
    lv_disp_set_rotation(screen, LV_DISP_ROT_90);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), 0);
    lv_obj_t * label = lv_label_create(screen);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_label_set_text(label, "Hello world");
}
