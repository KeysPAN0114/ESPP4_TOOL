/*
 * SPDX-FileCopyrightText: 2024 ESP32P4_TOOL
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * @file ui_menu.c
 * @brief 手机桌面风格功能菜单界面
 *
 * 屏幕分辨率: 800x480 (EK79007 + 90°旋转)
 * 布局: 状态栏 + 4x3 图标网格，共12个槽位（11个功能 + 1个空位）
 */

#include "ui_menu.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdint.h>

static const char *TAG = "ui_menu";

/* ======================== 屏幕参数定义 ======================== */
#define SCREEN_W            800
#define SCREEN_H            480

#define STATUS_BAR_H        50      /* 顶部状态栏高度 */
#define GRID_PADDING_X      16      /* 网格左右内边距 */
#define GRID_PADDING_Y      10      /* 网格顶部内边距 */
#define GRID_COLS           4       /* 列数 */
#define GRID_ROWS           3       /* 行数 */

#define ICON_SIZE           68      /* 图标圆角矩形尺寸 */
#define ICON_RADIUS         16      /* 图标圆角半径 */

/* 颜色主题 */
#define COLOR_BG            0x1A1A2E    /* 深色背景 */
#define COLOR_STATUS_BAR    0x16213E    /* 状态栏背景 */
#define COLOR_TEXT_WHITE     0xFFFFFF
#define COLOR_TEXT_LIGHT     0xB0B0B0
#define COLOR_ACCENT         0x00D2FF    /* 标题/强调色 */

/* ======================== 菜单数据 ======================== */

/* 菜单项数量 */
#define MENU_COUNT          11

/* 功能名称标签 */
static const char *menu_labels[MENU_COUNT] = {
    "Ammeter",          /* 电流表 */
    "Voltmeter",        /* 电压表 */
    "Logic\nAnalyzer",  /* 逻辑分析仪 */
    "Serial\nDebug",    /* 串口调试工具 */
    "BLE\nDebug",       /* 蓝牙调试工具 */
    "WiFi\nDebug",      /* WIFI调试工具 */
    "SPI\nDebug",       /* SPI调试工具 */
    "I2C\nDebug",       /* IIC调试工具 */
    "DSI\nDebug",       /* DSI调试工具 */
    "CSI\nDebug",       /* CSI调试工具 */
    "USB\nDebug",       /* USB调试工具 */
};

/* 图标内缩写文字 */
static const char *menu_icons[MENU_COUNT] = {
    "mA",   /* 电流表 */
    "V",    /* 电压表 */
    "LA",   /* 逻辑分析仪 */
    "TX",   /* 串口调试工具 */
    "BT",   /* 蓝牙调试工具 */
    "W",    /* WIFI调试工具 */
    "SPI",  /* SPI调试工具 */
    "I2C",  /* IIC调试工具 */
    "DSI",  /* DSI调试工具 */
    "CSI",  /* CSI调试工具 */
    "USB",  /* USB调试工具 */
};

/* 图标背景颜色 */
static const uint32_t menu_colors[MENU_COUNT] = {
    0x00B4D8,   /* 天蓝 - 电流表 */
    0xFF6B35,   /* 橙色 - 电压表 */
    0x7B2D8E,   /* 紫色 - 逻辑分析仪 */
    0x06D6A0,   /* 绿色 - 串口调试工具 */
    0x118AB2,   /* 蓝色 - 蓝牙调试工具 */
    0xEF476F,   /* 粉红 - WIFI调试工具 */
    0xFFD166,   /* 金色 - SPI调试工具 */
    0x06D6A0,   /* 绿色 - IIC调试工具 */
    0x8338EC,   /* 紫色 - DSI调试工具 */
    0x3A86FF,   /* 蓝色 - CSI调试工具 */
    0x00D2FF,   /* 青色 - USB调试工具 */
};

/* 全局提示条对象 */
static lv_obj_t *g_info_bar = NULL;

/* ======================== 回调函数 ======================== */

/**
 * @brief 提示条自动隐藏定时器回调
 */
static void info_bar_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *bar = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (bar) {
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_delete(timer);
}

/**
 * @brief 菜单项点击回调
 */
static void menu_item_click_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    const char *label = menu_labels[index];
    ESP_LOGI(TAG, "Clicked: [%d] %s", index, label);

    if (g_info_bar) {
        /* 更新提示文字 */
        lv_obj_t *info_label = lv_obj_get_child(g_info_bar, 0);
        if (info_label) {
            lv_label_set_text(info_label, menu_icons[index]);
        }
        /* 显示提示条 */
        lv_obj_clear_flag(g_info_bar, LV_OBJ_FLAG_HIDDEN);
        /* 2秒后自动隐藏 */
        lv_timer_create(info_bar_timer_cb, 2000, g_info_bar);
    }
}

/* ======================== 界面构建函数 ======================== */

/**
 * @brief 创建顶部状态栏
 */
static void create_status_bar(lv_obj_t *parent)
{
    /* 状态栏背景 */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCREEN_W, STATUS_BAR_H);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COLOR_STATUS_BAR), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 左侧标题 */
    lv_obj_t *title = lv_label_create(bar);
    lv_label_set_text(title, "ESP32P4 TOOL");
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

    /* 右侧副标题 */
    lv_obj_t *subtitle = lv_label_create(bar);
    lv_label_set_text(subtitle, "Select Tool");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(COLOR_TEXT_LIGHT), 0);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_align(subtitle, LV_ALIGN_RIGHT_MID, -15, 0);
}

/**
 * @brief 创建单个菜单图标
 *
 * @param parent   父对象
 * @param index    菜单项索引
 * @param col      列号 (0~3)
 * @param row      行号 (0~2)
 */
static void create_menu_icon(lv_obj_t *parent, int index, int col, int row)
{
    /* 计算单元格尺寸 */
    int content_w = SCREEN_W - (GRID_PADDING_X * 2);
    int cell_w = content_w / GRID_COLS;
    int content_h = SCREEN_H - STATUS_BAR_H - GRID_PADDING_Y - 5;
    int cell_h = content_h / GRID_ROWS;

    int x_pos = GRID_PADDING_X + col * cell_w;
    int y_pos = STATUS_BAR_H + GRID_PADDING_Y + row * cell_h;

    /* ---- 容器 ---- */
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, cell_w, cell_h);
    lv_obj_set_pos(container, x_pos, y_pos);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 图标按钮（圆角矩形） ---- */
    lv_obj_t *icon_btn = lv_btn_create(container);
    lv_obj_set_size(icon_btn, ICON_SIZE, ICON_SIZE);
    lv_obj_align(icon_btn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(icon_btn, lv_color_hex(menu_colors[index]), 0);
    lv_obj_set_style_bg_opa(icon_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(icon_btn, ICON_RADIUS, 0);
    lv_obj_set_style_shadow_width(icon_btn, 12, 0);
    lv_obj_set_style_shadow_opa(icon_btn, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(icon_btn, lv_color_hex(menu_colors[index]), 0);
    lv_obj_set_style_border_width(icon_btn, 0, 0);
    lv_obj_add_event_cb(icon_btn, menu_item_click_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)index);

    /* 图标缩写文字 */
    lv_obj_t *icon_label = lv_label_create(icon_btn);
    lv_label_set_text(icon_label, menu_icons[index]);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(COLOR_TEXT_WHITE), 0);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_20, 0);
    lv_obj_center(icon_label);

    /* ---- 功能名称标签 ---- */
    lv_obj_t *text_label = lv_label_create(container);
    lv_label_set_text(text_label, menu_labels[index]);
    lv_obj_set_style_text_color(text_label, lv_color_hex(COLOR_TEXT_LIGHT), 0);
    lv_obj_set_style_text_font(text_label, &lv_font_montserrat_14, 0);
    lv_obj_align(text_label, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(text_label, cell_w - 10);
    lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
}

/**
 * @brief 创建底部提示条
 */
static void create_info_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_btn_create(parent);
    lv_obj_set_size(bar, 300, 36);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333355), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_90, 0);
    lv_obj_set_style_radius(bar, 18, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x555577), 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *label = lv_label_create(bar);
    lv_label_set_text(label, "ESP32P4 TOOL");
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT_WHITE), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);

    g_info_bar = bar;
}

/* ======================== 主入口函数 ======================== */

void ui_menu_create(lv_display_t *disp)
{
    /* 初始化默认深色主题 */
    lv_theme_default_init(disp,
                          lv_palette_main(LV_PALETTE_CYAN),
                          lv_palette_main(LV_PALETTE_PURPLE),
                          LV_THEME_DEFAULT_DARK,
                          &lv_font_montserrat_16);

    lv_obj_t *screen = lv_display_get_screen_active(disp);

    /* 设置深色背景 */
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(COLOR_TEXT_WHITE), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 状态栏 */
    create_status_bar(screen);

    /* 底部提示条 */
    create_info_bar(screen);

    /* 网格菜单图标 */
    for (int i = 0; i < MENU_COUNT; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        create_menu_icon(screen, i, col, row);
    }

    ESP_LOGI(TAG, "Menu UI created: %d items, grid %dx%d", MENU_COUNT, GRID_COLS, GRID_ROWS);
}
