/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_efuse.h"
#include "driver/gpio.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"

#include "lvgl_port.h"

static const char *TAG = "lvgl_port";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// LVGL Port Configuration //////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Forward declarations for LCD parameters needed by LVGL port
// These must match the values in bsp_lcd.c
#if CONFIG_EXAMPLE_LCD_USE_ILI9881C
#define LVGL_PORT_LCD_H_RES    800
#define LVGL_PORT_LCD_V_RES    1280
#elif CONFIG_EXAMPLE_LCD_USE_EK79007
#define LVGL_PORT_LCD_H_RES    480
#define LVGL_PORT_LCD_V_RES    800
#endif

#define LVGL_PORT_DRAW_BUF_LINES    (LVGL_PORT_LCD_V_RES / 10) // number of display lines in each draw buffer
#define LVGL_PORT_TICK_PERIOD_MS    2
#define LVGL_PORT_TASK_STACK_SIZE   (16 * 1024)
#define LVGL_PORT_TASK_PRIORITY     2
#define LVGL_PORT_TASK_MAX_DELAY_MS 500
#define LVGL_PORT_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ

#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(num, align)  ((num) & ~((align) - 1))

#define EXAMPLE_LCD_ROTATION   90   // 0 / 90 / 270

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
#define PIN_NUM_REFRESH_MONITOR  20  // Monitor the Refresh Rate by toggling the GPIO
#endif

// LVGL logical resolution: swap width/height based on rotation
#if EXAMPLE_LCD_ROTATION == 90 || EXAMPLE_LCD_ROTATION == 270
    #define LVGL_PORT_HOR_RES  LVGL_PORT_LCD_V_RES   // 800
    #define LVGL_PORT_VER_RES  LVGL_PORT_LCD_H_RES   // 480
#else
    #define LVGL_PORT_HOR_RES  LVGL_PORT_LCD_H_RES
    #define LVGL_PORT_VER_RES  LVGL_PORT_LCD_V_RES
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Static Variables /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// LVGL library is not thread-safe, use a mutex to protect it
static _lock_t lvgl_api_lock;

// Rotation buffer for software rotation
static uint8_t rot_buf[LVGL_PORT_HOR_RES * LVGL_PORT_DRAW_BUF_LINES * 3];

// Touch panel handle
static esp_lcd_touch_handle_t tp_handle = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// LVGL Callbacks ///////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if CONFIG_EXAMPLE_USE_DMA2D_COPY_FRAME
static void lvgl_port_rounder_flush_area_cb(lv_event_t *event)
{
    lv_area_t *area = lv_event_get_invalidated_area(event);
    area->x1 = ALIGN_DOWN(area->x1, 16);
    area->x2 = ALIGN_UP(area->x2, 16) - 1;
}
#endif

static void lvgl_port_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);

#if EXAMPLE_LCD_ROTATION == 90
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    // logical coord -> physical coord (panel 480x800)
    int px1 = area->y1;                              // X = y
    int py1 = LVGL_PORT_HOR_RES - 1 - area->x2;     // Y = Lw-1-x
    // RGB888 per-pixel rotation
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint8_t *src = px_map + ((y - area->y1) * w + (x - area->x1)) * 3;
            uint8_t *dst = rot_buf + ((area->x2 - x) * h + (y - area->y1)) * 3;
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
        }
    }
    esp_lcd_panel_draw_bitmap(panel_handle, px1, py1, px1 + h, py1 + w, rot_buf);

#elif EXAMPLE_LCD_ROTATION == 270
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    int px1 = LVGL_PORT_VER_RES - 1 - area->y2;     // X = Lh-1-y
    int py1 = area->x1;                              // Y = x
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint8_t *src = px_map + ((y - area->y1) * w + (x - area->x1)) * 3;
            uint8_t *dst = rot_buf + ((y - area->y1) * w + (LVGL_PORT_VER_RES - 1 - x)) * 3;
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
        }
    }
    esp_lcd_panel_draw_bitmap(panel_handle, px1, py1, px1 + h, py1 + w, rot_buf);
#else
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);
#endif
}

static void lvgl_port_tick_cb(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Touch Input Callbacks /////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void lvgl_port_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t x[1];
    uint16_t y[1];
    uint16_t strength[1];
    uint8_t count = 0;

    esp_lcd_touch_read_data(tp_handle);
    if (esp_lcd_touch_get_coordinates(tp_handle, x, y, strength, &count, 1) && count > 0) {
#if EXAMPLE_LCD_ROTATION == 90
        // Map touch physical coords (480x800) to LVGL logical coords (800x480)
        data->point.x = (LVGL_PORT_HOR_RES - 1) - y[0];
        data->point.y = x[0];
#elif EXAMPLE_LCD_ROTATION == 270
        data->point.x = y[0];
        data->point.y = (LVGL_PORT_VER_RES - 1) - x[0];
#else
        data->point.x = x[0];
        data->point.y = y[0];
#endif
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGD(TAG, "Touch: raw(%d,%d) -> disp(%d,%d), state=PRESSED", x[0], y[0], data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, LVGL_PORT_TASK_MIN_DELAY_MS);
        // in case of lvgl display not ready yet
        time_till_next_ms = MIN(time_till_next_ms, LVGL_PORT_TASK_MAX_DELAY_MS);
        usleep(1000 * time_till_next_ms);
    }
}

static bool lvgl_port_notify_flush_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
static void lvgl_port_init_refresh_monitor_io(void)
{
    gpio_config_t monitor_io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_REFRESH_MONITOR,
    };
    ESP_ERROR_CHECK(gpio_config(&monitor_io_conf));
}

static bool lvgl_port_monitor_refresh_rate(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    static int io_level = 0;
    // please note, the real refresh rate should be 2*frequency of this GPIO toggling
    gpio_set_level(PIN_NUM_REFRESH_MONITOR, io_level);
    io_level = !io_level;
    return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Public API ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

lv_display_t *lvgl_port_init(esp_lcd_panel_handle_t panel_handle)
{
#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
    lvgl_port_init_refresh_monitor_io();
#endif

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // create a lvgl display
    lv_display_t *display = lv_display_create(LVGL_PORT_HOR_RES, LVGL_PORT_VER_RES);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);

    // create draw buffer
    void *buf1 = NULL;
    void *buf2 = NULL;
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers");

    size_t alignment = 1;
#if CONFIG_EXAMPLE_USE_DMA2D_COPY_FRAME
    if (esp_efuse_is_flash_encryption_enabled()) {
        alignment = SOC_GDMA_EXT_MEM_ENC_ALIGNMENT;
        if (LVGL_PORT_LCD_H_RES % alignment != 0) {
            ESP_LOGW(TAG, "LCD_H_RES is not aligned to %d, may cause MSPI error", alignment);
        }
    }
#endif
    size_t draw_buffer_sz = LVGL_PORT_HOR_RES * LVGL_PORT_DRAW_BUF_LINES * sizeof(lv_color_t);

    // Note:
    // Keep the display buffer in **internal** RAM can speed up the UI because LVGL uses it a lot and it should have a fast access time
    // This example allocate the buffer from PSRAM mainly because we want to save the internal RAM
    buf1 = heap_caps_aligned_calloc(alignment, 1, draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1);
    buf2 = heap_caps_aligned_calloc(alignment, 1, draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, lvgl_port_flush_cb);

#if CONFIG_EXAMPLE_USE_DMA2D_COPY_FRAME
    // If flash encryption is enabled, DMA2D requires the flush buffer address and size to be aligned to 16 bytes.
    // We need to round the flush area to the multiple of 16.
    if (esp_efuse_is_flash_encryption_enabled()) {
        ESP_LOGI(TAG, "Register event callback for LVGL flush area rounding");
        lv_display_add_event_cb(display, lvgl_port_rounder_flush_area_cb, LV_EVENT_INVALIDATE_AREA, NULL);
    }
#endif

    ESP_LOGI(TAG, "Register DPI panel event callback for LVGL flush ready notification");
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = lvgl_port_notify_flush_ready,
#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
        .on_refresh_done = lvgl_port_monitor_refresh_rate,
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(panel_handle, &cbs, display));

    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_port_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_PORT_TASK_STACK_SIZE, NULL, LVGL_PORT_TASK_PRIORITY, NULL);

    return display;
}

void lvgl_port_add_touch(esp_lcd_touch_handle_t touch_handle)
{
    tp_handle = touch_handle;

    ESP_LOGI(TAG, "Register touch input device");
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_port_touch_read_cb);
}

void lvgl_port_lock(void)
{
    _lock_acquire(&lvgl_api_lock);
}

void lvgl_port_unlock(void)
{
    _lock_release(&lvgl_api_lock);
}
