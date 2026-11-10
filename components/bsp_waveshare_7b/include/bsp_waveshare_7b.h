#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "driver/gpio.h"
#include "bsp_io_extension.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD Resolution & Timing Parameters (1024 x 600 RGB565, ST7262)
 *
 * Total H = 1024 + 162 + 152 + 48 = 1386 clocks
 * Total V = 600 + 45 + 13 + 3     = 661 lines
 * Clocks per frame = 1386 * 661   = 916,146 clocks
 * At 26 MHz PCLK: Native VSYNC refresh rate = 26,000,000 / 916,146 = ~28.38 FPS (Target: 25-30 FPS)
 */
#define BSP_LCD_H_RES                   (1024)
#define BSP_LCD_V_RES                   (600)
#define BSP_LCD_PCLK_HZ                 (26 * 1000 * 1000)

#define BSP_LCD_HSYNC_PULSE_WIDTH       (162)
#define BSP_LCD_HSYNC_BACK_PORCH        (152)
#define BSP_LCD_HSYNC_FRONT_PORCH       (48)
#define BSP_LCD_VSYNC_PULSE_WIDTH       (45)
#define BSP_LCD_VSYNC_BACK_PORCH        (13)
#define BSP_LCD_VSYNC_FRONT_PORCH       (3)

#define BSP_LCD_BITS_PER_PIXEL          (16)
#define BSP_LCD_DATA_WIDTH              (16)
#define BSP_LCD_FB_SIZE_BYTES           (BSP_LCD_H_RES * BSP_LCD_V_RES * 2)
#define BSP_LCD_BOUNCE_BUFFER_PX        (BSP_LCD_H_RES * 20)

/**
 * @brief RGB565 Control & Sync GPIOs
 */
#define BSP_LCD_GPIO_DISP               (GPIO_NUM_NC)
#define BSP_LCD_GPIO_VSYNC              (GPIO_NUM_3)
#define BSP_LCD_GPIO_HSYNC              (GPIO_NUM_46)
#define BSP_LCD_GPIO_DE                 (GPIO_NUM_5)
#define BSP_LCD_GPIO_PCLK               (GPIO_NUM_7)

/**
 * @brief RGB565 16-bit Data Bus GPIOs (B3..B7, G2..G7, R3..R7)
 */
#define BSP_LCD_GPIO_DATA0              (GPIO_NUM_14)   /*!< B3 */
#define BSP_LCD_GPIO_DATA1              (GPIO_NUM_38)   /*!< B4 */
#define BSP_LCD_GPIO_DATA2              (GPIO_NUM_18)   /*!< B5 */
#define BSP_LCD_GPIO_DATA3              (GPIO_NUM_17)   /*!< B6 */
#define BSP_LCD_GPIO_DATA4              (GPIO_NUM_10)   /*!< B7 */

#define BSP_LCD_GPIO_DATA5              (GPIO_NUM_39)   /*!< G2 */
#define BSP_LCD_GPIO_DATA6              (GPIO_NUM_0)    /*!< G3 */
#define BSP_LCD_GPIO_DATA7              (GPIO_NUM_45)   /*!< G4 */
#define BSP_LCD_GPIO_DATA8              (GPIO_NUM_48)   /*!< G5 */
#define BSP_LCD_GPIO_DATA9              (GPIO_NUM_47)   /*!< G6 */
#define BSP_LCD_GPIO_DATA10             (GPIO_NUM_21)   /*!< G7 */

#define BSP_LCD_GPIO_DATA11             (GPIO_NUM_1)    /*!< R3 */
#define BSP_LCD_GPIO_DATA12             (GPIO_NUM_2)    /*!< R4 */
#define BSP_LCD_GPIO_DATA13             (GPIO_NUM_42)   /*!< R5 */
#define BSP_LCD_GPIO_DATA14             (GPIO_NUM_41)   /*!< R6 */
#define BSP_LCD_GPIO_DATA15             (GPIO_NUM_40)   /*!< R7 */

/**
 * @brief Standard RGB565 Color Definitions
 */
#define BSP_COLOR_BLACK                 (0x0000)
#define BSP_COLOR_WHITE                 (0xFFFF)
#define BSP_COLOR_RED                   (0xF800)
#define BSP_COLOR_GREEN                 (0x07E0)
#define BSP_COLOR_BLUE                  (0x001F)
#define BSP_COLOR_YELLOW                (0xFFE0)
#define BSP_COLOR_CYAN                  (0x07FF)
#define BSP_COLOR_MAGENTA               (0xF81F)

/**
 * @brief Convert 8-bit R, G, B (0..255) to 16-bit RGB565.
 */
static inline uint16_t bsp_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/**
 * @brief Initialize the Waveshare ESP32-S3-Touch-LCD-7B display subsystem.
 *
 * Initializes I2C0 + IO Extension (0x24) if not already initialized, allocates
 * PSRAM framebuffers and internal SRAM bounce buffers, configures the ST7262
 * RGB565 timing parameters, clears the initial framebuffer to black, and turns
 * on the LCD backlight.
 *
 * @param[out] out_panel Optional pointer to receive the esp_lcd_panel_handle_t.
 * @return ESP_OK on success.
 */
esp_err_t bsp_display_init(esp_lcd_panel_handle_t *out_panel);

/**
 * @brief Turn the LCD backlight ON or OFF via IO Extension IO2.
 *
 * @param enable true = Backlight ON, false = Backlight OFF.
 * @return ESP_OK on success.
 */
esp_err_t bsp_display_set_backlight(bool enable);

/**
 * @brief Wait for the next RGB LCD VSYNC interrupt (for tear-free 25-30 FPS pacing).
 *
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return ESP_OK if VSYNC occurred, ESP_ERR_TIMEOUT on timeout.
 */
esp_err_t bsp_display_wait_vsync(uint32_t timeout_ms);

/**
 * @brief Fill the backbuffer with a solid RGB565 color and present it on screen.
 *
 * @param color_rgb565 16-bit RGB565 color value.
 * @return ESP_OK on success.
 */
esp_err_t bsp_display_fill_color(uint16_t color_rgb565);

/**
 * @brief Render a smooth horizontal/vertical color gradient frame into the backbuffer and present it.
 *
 * @param phase_step Animation phase step (0..255+) for smooth background color cycling.
 * @return ESP_OK on success.
 */
esp_err_t bsp_display_draw_background_sweep(uint32_t phase_step);

/**
 * @brief Render the full-screen 8-bar vertical color test pattern (White, Yellow, Cyan,
 *        Green, Magenta, Red, Blue, Black) with a 2px alignment border and present it.
 *
 * @return ESP_OK on success.
 */
esp_err_t bsp_display_draw_test_bars(void);

#ifdef __cplusplus
}
#endif
