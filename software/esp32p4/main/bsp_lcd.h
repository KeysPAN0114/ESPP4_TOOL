/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LCD hardware (MIPI DSI bus, DBI IO, DPI panel, backlight)
 *
 * This function initializes the complete LCD hardware stack including:
 * - MIPI DSI PHY power
 * - Backlight GPIO
 * - MIPI DSI bus
 * - DBI interface for commands
 * - DPI panel for display output
 * - Panel reset and initialization
 * - Backlight on
 *
 * @return esp_lcd_panel_handle_t The initialized LCD panel handle
 */
esp_lcd_panel_handle_t bsp_lcd_init(void);

/**
 * @brief Set LCD backlight level
 *
 * @param level Backlight level (high/low as defined by EXAMPLE_LCD_BK_LIGHT_ON_LEVEL)
 */
void bsp_lcd_backlight_set(uint32_t level);

#ifdef __cplusplus
}
#endif
