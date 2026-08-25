/*
 * SPDX-FileCopyrightText: 2024 ESP32P4_TOOL
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建手机桌面风格的功能菜单
 *
 * 在 800x480 屏幕上显示 4x3 网格的功能图标，
 * 包含11个工具: 电流表、电压表、逻辑分析仪、串口调试、
 * 蓝牙调试、WIFI调试、SPI调试、I2C调试、DSI调试、CSI调试、USB调试
 *
 * @param[in] disp LVGL 显示设备句柄
 */
void ui_menu_create(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
