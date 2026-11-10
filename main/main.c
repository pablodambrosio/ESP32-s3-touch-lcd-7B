#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "esp_clk_tree.h"
#include "bsp_waveshare_7b.h"

#if CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ != 240
#error "MCU CPU frequency must be configured to 240 MHz (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y)"
#endif

static const char *TAG = "bringup";

typedef struct {
    const char *name;
    uint16_t color_rgb565;
    uint32_t hold_frames;
} color_check_step_t;

void app_main(void)
{
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, " Waveshare ESP32-S3-Touch-LCD-7B — Spec 001 Board Bring-Up");
    ESP_LOGI(TAG, "==========================================================");

    uint32_t cpu_freq_hz = 0;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(
        SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &cpu_freq_hz));
    size_t psram_size = esp_psram_get_size();
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Clock & Memory: CPU=%" PRIu32 " MHz, PSRAM total=%u KB (free=%u KB), Internal SRAM free=%u KB",
             cpu_freq_hz / 1000000U,
             (unsigned)(psram_size / 1024),
             (unsigned)(free_psram / 1024),
             (unsigned)(free_sram / 1024));

    // 1. Initialize I2C0, IO Extension (0x24), and 1024x600 RGB565 LCD panel
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(bsp_display_init(&panel_handle));

    // 2. Phase 1A: Solid Background Color Check (Red, Green, Blue, White, Black) paced by VSYNC (~28.4 FPS)
    static const color_check_step_t k_solid_checks[] = {
        {"PURE RED   (R3..R7)", BSP_COLOR_RED,   28},
        {"PURE GREEN (G2..G7)", BSP_COLOR_GREEN, 28},
        {"PURE BLUE  (B3..B7)", BSP_COLOR_BLUE,  28},
        {"PURE WHITE (ALL 16)", BSP_COLOR_WHITE, 28},
        {"PURE BLACK (BL CHK)", BSP_COLOR_BLACK, 20},
    };

    ESP_LOGI(TAG, "Phase 1A: Starting solid background color check...");
    for (size_t s = 0; s < sizeof(k_solid_checks) / sizeof(k_solid_checks[0]); s++) {
        ESP_LOGI(TAG, "  -> Displaying %s (0x%04X)", k_solid_checks[s].name, k_solid_checks[s].color_rgb565);
        int64_t t_start_us = esp_timer_get_time();

        for (uint32_t f = 0; f < k_solid_checks[s].hold_frames; f++) {
            ESP_ERROR_CHECK(bsp_display_fill_color(k_solid_checks[s].color_rgb565));
            (void)bsp_display_wait_vsync(100);
        }

        int64_t elapsed_us = esp_timer_get_time() - t_start_us;
        double fps = (double)k_solid_checks[s].hold_frames * 1e6 / (double)elapsed_us;
        ESP_LOGI(TAG, "     Completed %" PRIu32 " frames @ %.2f FPS", k_solid_checks[s].hold_frames, fps);
    }

    // 3. Phase 1B: Smooth Animated Background Color Sweep (120 frames @ ~28.4 FPS)
    const uint32_t sweep_frames = 120;
    ESP_LOGI(TAG, "Phase 1B: Running smooth background color sweep (%" PRIu32 " frames, target 25-30 FPS)...",
             sweep_frames);

    int64_t batch_start_us = esp_timer_get_time();
    int64_t sweep_start_us = batch_start_us;

    for (uint32_t frame = 1; frame <= sweep_frames; frame++) {
        ESP_ERROR_CHECK(bsp_display_draw_background_sweep(frame));
        (void)bsp_display_wait_vsync(100);

        if ((frame % 30U) == 0U) {
            int64_t now_us = esp_timer_get_time();
            double window_s = (double)(now_us - batch_start_us) / 1e6;
            double fps = 30.0 / window_s;
            double frame_ms = (window_s * 1000.0) / 30.0;
            ESP_LOGI(TAG, "Background sweep [frame %3" PRIu32 "/%" PRIu32 "] FPS: %.2f (avg frame time: %.2f ms)",
                     frame, sweep_frames, fps, frame_ms);
            batch_start_us = now_us;
        }
    }

    int64_t total_sweep_us = esp_timer_get_time() - sweep_start_us;
    double avg_sweep_fps = (double)sweep_frames * 1e6 / (double)total_sweep_us;
    ESP_LOGI(TAG, "Phase 1 Complete — Average Background Sweep FPS: %.2f FPS", avg_sweep_fps);

    // 4. Phase 2: Finalize with 8-Bar Vertical Color Test Pattern (1024x600)
    ESP_LOGI(TAG, "Phase 2: Rendering final 8-bar vertical color test pattern (1024x600)...");
    ESP_ERROR_CHECK(bsp_display_draw_test_bars());
    (void)bsp_display_wait_vsync(100);
    // Copy test bars to the second framebuffer as well so both buffers hold the identical final pattern
    ESP_ERROR_CHECK(bsp_display_draw_test_bars());

    ESP_LOGI(TAG, "Bring-up complete. Holding 8-bar test pattern on screen.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
