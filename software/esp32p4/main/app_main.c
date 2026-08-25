/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_err.h"
#include "esp_log.h"

#include "bsp_lcd.h"
#include "bsp_lcd_touch.h"
#include "lvgl_port.h"

static const char *TAG = "app_main";

#include "ui_menu.h"

void app_main(void)
{
    // Initialize LCD hardware (MIPI DSI bus, panel, backlight)
    esp_lcd_panel_handle_t panel = bsp_lcd_init();

    // Initialize GT911 touch controller
    esp_lcd_touch_handle_t tp = bsp_lcd_touch_init();

    // Initialize LVGL library, display, buffers, tick timer, and LVGL task
    lv_display_t *display = lvgl_port_init(panel);

    // Register touch input device with LVGL
    lvgl_port_add_touch(tp);

    // Display the main menu UI
    ESP_LOGI(TAG, "Display Main Menu UI");
    lvgl_port_lock();
    ui_menu_create(display);
    lvgl_port_unlock();
}
