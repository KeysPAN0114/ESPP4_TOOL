/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL library and create display with LVGL port
 *
 * This function initializes the LVGL library, creates a display object,
 * allocates draw buffers, sets up flush callback, tick timer, and starts
 * the LVGL handler task.
 *
 * @param[in] panel_handle The LCD panel handle (from bsp_lcd_init)
 * @return lv_display_t* The created LVGL display handle
 */
lv_display_t *lvgl_port_init(esp_lcd_panel_handle_t panel_handle);

/**
 * @brief Add touch input device to LVGL
 *
 * This function registers the touch controller as an LVGL input device.
 * Must be called after lvgl_port_init().
 *
 * @param[in] tp_handle The touch panel handle (from bsp_lcd_touch_init)
 */
void lvgl_port_add_touch(esp_lcd_touch_handle_t tp_handle);

/**
 * @brief Acquire the LVGL API lock
 *
 * Must be called before calling any LVGL API from a non-LVGL task.
 */
void lvgl_port_lock(void);

/**
 * @brief Release the LVGL API lock
 *
 * Must be called after finishing LVGL API calls.
 */
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif
