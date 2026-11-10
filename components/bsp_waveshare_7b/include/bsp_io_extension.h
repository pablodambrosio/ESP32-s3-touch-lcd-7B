#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C0 Master Bus Configuration (Waveshare ESP32-S3-Touch-LCD-7B)
 */
#define BSP_I2C_PORT_NUM                (I2C_NUM_0)
#define BSP_I2C_SDA_GPIO                (GPIO_NUM_8)
#define BSP_I2C_SCL_GPIO                (GPIO_NUM_9)
#define BSP_I2C_FREQ_HZ                 (400000)

/**
 * @brief Onboard I2C IO Extension Slave Address & Registers
 */
#define BSP_IO_EXTENSION_I2C_ADDR       (0x24)

#define BSP_IO_EXT_REG_MODE             (0x02)  /*!< Direction mode register (1 = output, 0 = input) */
#define BSP_IO_EXT_REG_OUTPUT           (0x03)  /*!< Output level register (1 = high, 0 = low) */
#define BSP_IO_EXT_REG_INPUT            (0x04)  /*!< Input level register */
#define BSP_IO_EXT_REG_PWM              (0x05)  /*!< Backlight PWM duty register */
#define BSP_IO_EXT_REG_ADC              (0x06)  /*!< 12-bit Battery ADC read register */

/**
 * @brief IO Extension Pin Numbers (0..7)
 */
#define BSP_IO_EXT_PIN_IO0              (0)     /*!< Expansion IO0 */
#define BSP_IO_EXT_PIN_TOUCH_RST        (1)     /*!< IO1: GT911 Touch Reset (0 = reset, 1 = normal) */
#define BSP_IO_EXT_PIN_LCD_BL           (2)     /*!< IO2: LCD Backlight enable (1 = ON, 0 = OFF) */
#define BSP_IO_EXT_PIN_LCD_RST          (3)     /*!< IO3: LCD Reset (1 = normal) */
#define BSP_IO_EXT_PIN_SD_CS            (4)     /*!< IO4: TF/SD Card CS (1 = enabled) */
#define BSP_IO_EXT_PIN_USB_CAN_SEL      (5)     /*!< IO5: 0 = USB, 1 = CAN (TWAI) */
#define BSP_IO_EXT_PIN_IO6              (6)     /*!< Expansion IO6 */
#define BSP_IO_EXT_PIN_IO7              (7)     /*!< Expansion IO7 */

/**
 * @brief Initialize I2C0 master bus and onboard IO Extension chip (0x24).
 *
 * Sets all IO extension pins to output mode with safe initial levels:
 * - LCD Backlight (IO2) = 0 (OFF until initial framebuffer is ready)
 * - LCD Reset (IO3)     = 1 (Released)
 * - Touch Reset (IO1)   = 1 (Released)
 * - SD CS (IO4)         = 1 (High)
 * - USB/CAN Mux (IO5)   = 0 (USB selected)
 *
 * @return ESP_OK on success, or an ESP-IDF error code.
 */
esp_err_t bsp_io_extension_init(void);

/**
 * @brief Get the initialized I2C0 master bus handle (for sharing with GT911 or external sensors).
 *
 * @return i2c_master_bus_handle_t or NULL if not initialized.
 */
i2c_master_bus_handle_t bsp_i2c_get_bus_handle(void);

/**
 * @brief Set the output level of a single pin on the IO Extension chip.
 *
 * @param pin   Pin index (0..7, e.g., BSP_IO_EXT_PIN_LCD_BL)
 * @param level Output logic level (true = High / 1, false = Low / 0)
 * @return ESP_OK on success.
 */
esp_err_t bsp_io_extension_set_level(uint8_t pin, bool level);

/**
 * @brief Read the input level of a single pin on the IO Extension chip.
 *
 * @param pin       Pin index (0..7)
 * @param out_level Pointer to store read logic level
 * @return ESP_OK on success.
 */
esp_err_t bsp_io_extension_get_level(uint8_t pin, bool *out_level);

/**
 * @brief Set the PWM duty cycle on the IO Extension chip.
 *
 * @param duty_percent Brightness percentage (0..100, clamped at 97 per hardware spec)
 * @return ESP_OK on success.
 */
esp_err_t bsp_io_extension_set_pwm(uint8_t duty_percent);

/**
 * @brief Read the raw 12-bit ADC value from the IO Extension chip (battery sense).
 *
 * @param out_adc Pointer to store the 16-bit raw reading
 * @return ESP_OK on success.
 */
esp_err_t bsp_io_extension_read_adc(uint16_t *out_adc);

#ifdef __cplusplus
}
#endif
