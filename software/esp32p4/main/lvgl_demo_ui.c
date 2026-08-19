/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"
#include <stdio.h>

static lv_style_t style_bullet;
static const lv_font_t *font_normal = &lv_font_montserrat_16;

static int click_count = 0;
static lv_obj_t *count_label = NULL;

static void btn_click_cb(lv_event_t *e)
{
    (void)e;
    click_count++;
    char buf[32];
    snprintf(buf, sizeof(buf), "Clicked: %d", click_count);
    lv_label_set_text(count_label, buf);
}

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
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), 0);

    // Title label
    lv_obj_t *title_label = lv_label_create(screen);
    lv_obj_set_align(title_label, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_top(title_label, 40, 0);
    lv_label_set_text(title_label, "Touch Button Test");

    // Create a button
    lv_obj_t *btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_shadow_width(btn, 20, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

    // Button label
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_label);

    // Count display label (positioned below button)
    count_label = lv_label_create(screen);
    lv_obj_set_align(count_label, LV_ALIGN_CENTER);
    lv_obj_set_y(count_label, 80);
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(count_label, "Clicked: 0");
}
