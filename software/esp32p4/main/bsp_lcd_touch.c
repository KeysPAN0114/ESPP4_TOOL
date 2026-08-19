/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/i2c_master.h"

#include "bsp_lcd_touch.h"

static const char *TAG = "bsp_touch";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Touch Hardware Configuration /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TOUCH_I2C_SCL_PIN       8
#define TOUCH_I2C_SDA_PIN       7
#define TOUCH_I2C_CLK_HZ       400000
#define TOUCH_INT_PIN           21

// Touch resolution (match your LCD resolution)
#define TOUCH_X_MAX             480
#define TOUCH_Y_MAX             800

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Public API ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

esp_lcd_touch_handle_t bsp_lcd_touch_init(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus for touch");

    // Configure I2C bus using new master driver
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = TOUCH_I2C_SDA_PIN,
        .scl_io_num = TOUCH_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    ESP_LOGI(TAG, "Initialize GT911 touch controller");

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

    esp_lcd_touch_config_t tp_config = {
        .x_max = TOUCH_X_MAX,
        .y_max = TOUCH_Y_MAX,
        .rst_gpio_num = -1,  // No reset pin connected
        .int_gpio_num = TOUCH_INT_PIN,
    };

    esp_lcd_touch_handle_t tp_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(io_handle, &tp_config, &tp_handle));

    ESP_LOGI(TAG, "GT911 touch controller initialized successfully");

    return tp_handle;
}
