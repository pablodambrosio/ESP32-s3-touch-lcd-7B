#include "bsp_io_extension.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bsp_i2c_io";

static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;
static i2c_master_dev_handle_t s_io_ext_dev_handle = NULL;
static uint8_t s_output_latch = 0xFF;

static esp_err_t io_ext_write_reg(uint8_t reg_addr, uint8_t val)
{
    ESP_RETURN_ON_FALSE(s_io_ext_dev_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "IO Extension not initialized");
    uint8_t payload[2] = {reg_addr, val};
    return i2c_master_transmit(s_io_ext_dev_handle, payload, sizeof(payload), 100);
}

esp_err_t bsp_io_extension_init(void)
{
    if (s_io_ext_dev_handle != NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing I2C0 master bus (SDA=GPIO%d, SCL=GPIO%d @ %d Hz)",
             BSP_I2C_SDA_GPIO, BSP_I2C_SCL_GPIO, BSP_I2C_FREQ_HZ);

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = BSP_I2C_PORT_NUM,
        .sda_io_num = BSP_I2C_SDA_GPIO,
        .scl_io_num = BSP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus_handle), TAG, "Failed to create I2C0 master bus");

    const i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BSP_IO_EXTENSION_I2C_ADDR,
        .scl_speed_hz = BSP_I2C_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_i2c_bus_handle, &dev_config, &s_io_ext_dev_handle),
                        TAG, "Failed to add IO Extension device (0x%02X)", BSP_IO_EXTENSION_I2C_ADDR);

    // Configure all 8 IO expander pins as outputs (0xFF)
    ESP_RETURN_ON_ERROR(io_ext_write_reg(BSP_IO_EXT_REG_MODE, 0xFF),
                        TAG, "Failed to write IO Extension mode register");

    /*
     * Safe initial output state:
     * - IO1 (Touch RST) = 1 (Released)
     * - IO2 (LCD BL)    = 0 (Backlight OFF until first frame is ready)
     * - IO3 (LCD RST)   = 1 (LCD active)
     * - IO4 (SD CS)     = 1 (Deselected)
     * - IO5 (USB/CAN)   = 0 (USB selected)
     */
    s_output_latch = (uint8_t)((1U << BSP_IO_EXT_PIN_IO0) |
                               (1U << BSP_IO_EXT_PIN_TOUCH_RST) |
                               (0U << BSP_IO_EXT_PIN_LCD_BL) |
                               (1U << BSP_IO_EXT_PIN_LCD_RST) |
                               (1U << BSP_IO_EXT_PIN_SD_CS) |
                               (0U << BSP_IO_EXT_PIN_USB_CAN_SEL) |
                               (1U << BSP_IO_EXT_PIN_IO6) |
                               (1U << BSP_IO_EXT_PIN_IO7));

    ESP_RETURN_ON_ERROR(io_ext_write_reg(BSP_IO_EXT_REG_OUTPUT, s_output_latch),
                        TAG, "Failed to write initial IO Extension output state");

    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_LOGI(TAG, "IO Extension (0x%02X) initialized (latch=0x%02X)", BSP_IO_EXTENSION_I2C_ADDR, s_output_latch);
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_bus_handle(void)
{
    return s_i2c_bus_handle;
}

esp_err_t bsp_io_extension_set_level(uint8_t pin, bool level)
{
    ESP_RETURN_ON_FALSE(pin < 8, ESP_ERR_INVALID_ARG, TAG, "Invalid IO Extension pin %u", pin);
    if (level) {
        s_output_latch |= (uint8_t)(1U << pin);
    } else {
        s_output_latch &= (uint8_t)~(1U << pin);
    }
    return io_ext_write_reg(BSP_IO_EXT_REG_OUTPUT, s_output_latch);
}

esp_err_t bsp_io_extension_get_level(uint8_t pin, bool *out_level)
{
    ESP_RETURN_ON_FALSE(pin < 8 && out_level != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");
    ESP_RETURN_ON_FALSE(s_io_ext_dev_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "IO Extension not initialized");

    uint8_t reg = BSP_IO_EXT_REG_INPUT;
    uint8_t val = 0;
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(s_io_ext_dev_handle, &reg, 1, &val, 1, 100),
                        TAG, "Failed to read IO Extension input register");
    *out_level = ((val & (1U << pin)) != 0);
    return ESP_OK;
}

esp_err_t bsp_io_extension_set_pwm(uint8_t duty_percent)
{
    if (duty_percent >= 97) {
        duty_percent = 97;
    }
    uint8_t pwm_raw = (uint8_t)((duty_percent * 255U) / 100U);
    return io_ext_write_reg(BSP_IO_EXT_REG_PWM, pwm_raw);
}

esp_err_t bsp_io_extension_read_adc(uint16_t *out_adc)
{
    ESP_RETURN_ON_FALSE(out_adc != NULL, ESP_ERR_INVALID_ARG, TAG, "out_adc is NULL");
    ESP_RETURN_ON_FALSE(s_io_ext_dev_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "IO Extension not initialized");

    uint8_t reg = BSP_IO_EXT_REG_ADC;
    uint8_t rx_buf[2] = {0, 0};
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(s_io_ext_dev_handle, &reg, 1, rx_buf, sizeof(rx_buf), 100),
                        TAG, "Failed to read IO Extension ADC register");
    *out_adc = (uint16_t)((rx_buf[1] << 8) | rx_buf[0]);
    return ESP_OK;
}
