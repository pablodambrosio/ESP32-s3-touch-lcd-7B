#include "bsp_waveshare_7b.h"
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "bsp_lcd";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static uint16_t *s_frame_buffers[2] = {NULL, NULL};
static uint8_t s_back_fb_idx = 0;
static SemaphoreHandle_t s_vsync_sem = NULL;

IRAM_ATTR static bool bsp_rgb_on_vsync_isr(esp_lcd_panel_handle_t panel,
                                           const esp_lcd_rgb_panel_event_data_t *edata,
                                           void *user_ctx)
{
    (void)panel;
    (void)edata;
    (void)user_ctx;

    BaseType_t high_task_woken = pdFALSE;
    if (s_vsync_sem != NULL) {
        xSemaphoreGiveFromISR(s_vsync_sem, &high_task_woken);
    }
    return (high_task_woken == pdTRUE);
}

static esp_err_t bsp_present_backbuffer(void)
{
    uint16_t *fb = s_frame_buffers[s_back_fb_idx];

    // Write back CPU data cache to Octal PSRAM so GDMA bounce-buffer refill reads coherent pixels
    esp_cache_msync(fb, BSP_LCD_FB_SIZE_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

    // Passing the internal PSRAM framebuffer pointer switches cur_fb_index without CPU memcpy
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, fb),
                        TAG, "Failed to present framebuffer");

    // Flip to the other PSRAM framebuffer for the next frame
    s_back_fb_idx ^= 1U;
    return ESP_OK;
}

esp_err_t bsp_display_init(esp_lcd_panel_handle_t *out_panel)
{
    if (s_panel_handle != NULL) {
        if (out_panel != NULL) {
            *out_panel = s_panel_handle;
        }
        return ESP_OK;
    }

    // 1. Ensure I2C0 and IO Extension (0x24) are initialized (Backlight starts OFF)
    ESP_RETURN_ON_ERROR(bsp_io_extension_init(), TAG, "IO Extension init failed");

    // 2. Create binary semaphore for VSYNC synchronization
    if (s_vsync_sem == NULL) {
        s_vsync_sem = xSemaphoreCreateBinary();
        ESP_RETURN_ON_FALSE(s_vsync_sem != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create VSYNC semaphore");
    }

    // 3. Configure RGB565 LCD panel (1024x600, ST7262, 2 framebuffers in Octal PSRAM + SRAM bounce buffer)
    ESP_LOGI(TAG, "Installing RGB LCD panel driver (%dx%d @ %.2f MHz PCLK)",
             BSP_LCD_H_RES, BSP_LCD_V_RES, (double)BSP_LCD_PCLK_HZ / 1e6);

    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = BSP_LCD_PCLK_HZ,
            .h_res = BSP_LCD_H_RES,
            .v_res = BSP_LCD_V_RES,
            .hsync_pulse_width = BSP_LCD_HSYNC_PULSE_WIDTH,
            .hsync_back_porch = BSP_LCD_HSYNC_BACK_PORCH,
            .hsync_front_porch = BSP_LCD_HSYNC_FRONT_PORCH,
            .vsync_pulse_width = BSP_LCD_VSYNC_PULSE_WIDTH,
            .vsync_back_porch = BSP_LCD_VSYNC_BACK_PORCH,
            .vsync_front_porch = BSP_LCD_VSYNC_FRONT_PORCH,
            .flags = {
                .pclk_active_neg = 1,
            },
        },
        .data_width = BSP_LCD_DATA_WIDTH,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 2,
        .bounce_buffer_size_px = BSP_LCD_BOUNCE_BUFFER_PX,
        .dma_burst_size = 64,
        .hsync_gpio_num = BSP_LCD_GPIO_HSYNC,
        .vsync_gpio_num = BSP_LCD_GPIO_VSYNC,
        .de_gpio_num = BSP_LCD_GPIO_DE,
        .pclk_gpio_num = BSP_LCD_GPIO_PCLK,
        .disp_gpio_num = BSP_LCD_GPIO_DISP,
        .data_gpio_nums = {
            BSP_LCD_GPIO_DATA0,
            BSP_LCD_GPIO_DATA1,
            BSP_LCD_GPIO_DATA2,
            BSP_LCD_GPIO_DATA3,
            BSP_LCD_GPIO_DATA4,
            BSP_LCD_GPIO_DATA5,
            BSP_LCD_GPIO_DATA6,
            BSP_LCD_GPIO_DATA7,
            BSP_LCD_GPIO_DATA8,
            BSP_LCD_GPIO_DATA9,
            BSP_LCD_GPIO_DATA10,
            BSP_LCD_GPIO_DATA11,
            BSP_LCD_GPIO_DATA12,
            BSP_LCD_GPIO_DATA13,
            BSP_LCD_GPIO_DATA14,
            BSP_LCD_GPIO_DATA15,
        },
        .flags = {
            .fb_in_psram = 1,
        },
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle), TAG, "esp_lcd_new_rgb_panel failed");

    const esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = bsp_rgb_on_vsync_isr,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_register_event_callbacks(s_panel_handle, &cbs, NULL),
                        TAG, "Failed to register VSYNC callback");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel_handle), TAG, "esp_lcd_panel_reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel_handle), TAG, "esp_lcd_panel_init failed");

    // Retrieve both PSRAM framebuffers allocated by the RGB driver
    void *fb0 = NULL;
    void *fb1 = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_get_frame_buffer(s_panel_handle, 2, &fb0, &fb1),
                        TAG, "Failed to get RGB frame buffers");
    s_frame_buffers[0] = (uint16_t *)fb0;
    s_frame_buffers[1] = (uint16_t *)fb1;
    s_back_fb_idx = 0;

    // Clear both framebuffers to black before enabling backlight
    memset(s_frame_buffers[0], 0, BSP_LCD_FB_SIZE_BYTES);
    memset(s_frame_buffers[1], 0, BSP_LCD_FB_SIZE_BYTES);
    esp_cache_msync(s_frame_buffers[0], BSP_LCD_FB_SIZE_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    esp_cache_msync(s_frame_buffers[1], BSP_LCD_FB_SIZE_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

    // Wait for one VSYNC frame so clean black pixels are clocked into the panel, then enable backlight
    (void)bsp_display_wait_vsync(100);
    ESP_RETURN_ON_ERROR(bsp_display_set_backlight(true), TAG, "Failed to turn on LCD backlight");

    ESP_LOGI(TAG, "RGB LCD initialized: FB0=%p, FB1=%p (%u KB each in PSRAM)",
             fb0, fb1, (unsigned)(BSP_LCD_FB_SIZE_BYTES / 1024));

    if (out_panel != NULL) {
        *out_panel = s_panel_handle;
    }
    return ESP_OK;
}

esp_err_t bsp_display_set_backlight(bool enable)
{
    return bsp_io_extension_set_level(BSP_IO_EXT_PIN_LCD_BL, enable);
}

esp_err_t bsp_display_wait_vsync(uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(s_vsync_sem != NULL, ESP_ERR_INVALID_STATE, TAG, "Display not initialized");
    // Drain any stale VSYNC token so we synchronize with the next fresh VSYNC edge
    xSemaphoreTake(s_vsync_sem, 0);
    if (xSemaphoreTake(s_vsync_sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t bsp_display_fill_color(uint16_t color_rgb565)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    uint32_t *fb32 = (uint32_t *)s_frame_buffers[s_back_fb_idx];
    const uint32_t packed_two_px = ((uint32_t)color_rgb565 << 16) | (uint32_t)color_rgb565;
    const size_t total_words = (BSP_LCD_H_RES * BSP_LCD_V_RES) / 2;

    for (size_t i = 0; i < total_words; i++) {
        fb32[i] = packed_two_px;
    }

    return bsp_present_backbuffer();
}

/**
 * @brief Fast integer HSV-like color wheel helper (hue 0..767 -> RGB888)
 */
static inline void hue_to_rgb(uint16_t hue, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint16_t h = hue % 768U;
    uint8_t sector = (uint8_t)(h >> 8);      // 0, 1, or 2
    uint8_t offset = (uint8_t)(h & 0xFFU);   // 0..255

    switch (sector) {
        case 0:
            *r = (uint8_t)(255U - offset);
            *g = offset;
            *b = 48U;
            break;
        case 1:
            *r = 48U;
            *g = (uint8_t)(255U - offset);
            *b = offset;
            break;
        default:
            *r = offset;
            *g = 48U;
            *b = (uint8_t)(255U - offset);
            break;
    }
}

esp_err_t bsp_display_draw_background_sweep(uint32_t phase_step)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    uint32_t *fb32 = (uint32_t *)s_frame_buffers[s_back_fb_idx];
    const size_t words_per_row = BSP_LCD_H_RES / 2;
    const uint16_t base_hue = (uint16_t)((phase_step * 8U) % 768U);

    // Smooth vertical background gradient that sweeps smoothly through the color wheel at 25-30 FPS
    for (int y = 0; y < BSP_LCD_V_RES; y++) {
        uint8_t r, g, b;
        uint16_t row_hue = (uint16_t)((base_hue + (y / 2)) % 768U);
        hue_to_rgb(row_hue, &r, &g, &b);

        uint16_t color = bsp_rgb565(r, g, b);
        uint32_t packed = ((uint32_t)color << 16) | (uint32_t)color;
        uint32_t *row_ptr = fb32 + ((size_t)y * words_per_row);

        for (size_t w = 0; w < words_per_row; w++) {
            row_ptr[w] = packed;
        }
    }

    return bsp_present_backbuffer();
}

esp_err_t bsp_display_draw_test_bars(void)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    static const uint16_t k_bar_colors[8] = {
        BSP_COLOR_WHITE,    // Bar 0: 0..127
        BSP_COLOR_YELLOW,   // Bar 1: 128..255
        BSP_COLOR_CYAN,     // Bar 2: 256..383
        BSP_COLOR_GREEN,    // Bar 3: 384..511
        BSP_COLOR_MAGENTA,  // Bar 4: 512..639
        BSP_COLOR_RED,      // Bar 5: 640..767
        BSP_COLOR_BLUE,     // Bar 6: 768..895
        BSP_COLOR_BLACK,    // Bar 7: 896..1023
    };

    uint16_t *fb = s_frame_buffers[s_back_fb_idx];
    const int bar_width = BSP_LCD_H_RES / 8; // 128 pixels per bar

    // Build one scanline of 8 color bars first, then replicate across all 600 lines
    uint16_t *first_row = fb;
    for (int bar = 0; bar < 8; bar++) {
        uint16_t color = k_bar_colors[bar];
        int x_start = bar * bar_width;
        for (int x = 0; x < bar_width; x++) {
            first_row[x_start + x] = color;
        }
    }

    for (int y = 1; y < BSP_LCD_V_RES; y++) {
        memcpy(fb + ((size_t)y * BSP_LCD_H_RES), first_row, BSP_LCD_H_RES * sizeof(uint16_t));
    }

    // Draw a crisp 2-pixel white alignment border around the full 1024x600 perimeter
    for (int x = 0; x < BSP_LCD_H_RES; x++) {
        fb[0 * BSP_LCD_H_RES + x] = BSP_COLOR_WHITE;
        fb[1 * BSP_LCD_H_RES + x] = BSP_COLOR_WHITE;
        fb[(BSP_LCD_V_RES - 2) * BSP_LCD_H_RES + x] = BSP_COLOR_WHITE;
        fb[(BSP_LCD_V_RES - 1) * BSP_LCD_H_RES + x] = BSP_COLOR_WHITE;
    }
    for (int y = 0; y < BSP_LCD_V_RES; y++) {
        fb[y * BSP_LCD_H_RES + 0] = BSP_COLOR_WHITE;
        fb[y * BSP_LCD_H_RES + 1] = BSP_COLOR_WHITE;
        fb[y * BSP_LCD_H_RES + (BSP_LCD_H_RES - 2)] = BSP_COLOR_WHITE;
        fb[y * BSP_LCD_H_RES + (BSP_LCD_H_RES - 1)] = BSP_COLOR_WHITE;
    }

    return bsp_present_backbuffer();
}
