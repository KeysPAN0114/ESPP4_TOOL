/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize GT911 touch controller via I2C
 *
 * This function initializes the I2C bus and GT911 touch controller.
 *
 * @return esp_lcd_touch_handle_t The initialized touch handle
 */
esp_lcd_touch_handle_t bsp_lcd_touch_init(void);

#ifdef __cplusplus
}
#endif
