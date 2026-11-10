# Hardware Specification & Pinout — Waveshare ESP32-S3-Touch-LCD-7B

Reference: [Waveshare ESP32-S3-Touch-LCD-7B Wiki](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B) & Official ESP-IDF BSP Examples.

---

## 1. Core Module Specifications

- **Module**: `ESP32-S3-WROOM-1-N16R8`
- **CPU**: Dual-core Xtensa® 32-bit LX7, up to **240 MHz**
- **SRAM**: 512 KB Internal SRAM
- **PSRAM**: **8 MB Octal SPI PSRAM** (3.3V, uses `GPIO33`–`GPIO37` internally)
- **Flash**: **16 MB Quad SPI Flash** (uses `GPIO26`–`GPIO32` internally)
- **Display**: 7-inch IPS TFT, **1024 × 600** pixels, 16-bit RGB565 (65K colors), **ST7262** controller, 235 cd/m²
- **Touch Controller**: **GT911** 5-point capacitive touch via I2C

> **CRITICAL**: Never assign `GPIO26` through `GPIO37` in firmware—they are dedicated to the internal Flash and Octal PSRAM bus.

---

## 2. I2C Bus (`I2C_NUM_0`)

The internal and external I2C peripherals share `I2C_NUM_0`:

| Signal | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- |
| `I2C_SDA` | `GPIO_NUM_8` | Shared by IO Extension (`0x24`), GT911 Touch (`0x5D`/`0x14`), and I2C Header |
| `I2C_SCL` | `GPIO_NUM_9` | Recommended clock speed: **400 kHz** (`400000` Hz), glitch filter = `7` |

---

## 3. Onboard I2C IO Extension Chip (Address `0x24`)

The board integrates an I2C IO Expansion MCU at 7-bit slave address **`0x24`** that manages reset lines, backlight PWM, SD card CS, USB/CAN multiplexing, and battery voltage sensing.

### 3.1 Register Map (Slave Address `0x24`)

| Register | Name | Access | Description |
| :--- | :--- | :--- | :--- |
| `0x02` | `IO_EXTENSION_Mode` | Write (1 byte) | Bitmask direction mode (`1` = Output, `0` = Input). Write `0xFF` to set all pins as outputs. |
| `0x03` | `IO_EXTENSION_IO_OUTPUT_ADDR` | Write (1 byte) | Bitmask output level (`1` = High, `0` = Low) for `IO0`–`IO7`. |
| `0x04` | `IO_EXTENSION_IO_INPUT_ADDR` | Read (1 byte) | Bitmask input level readback for `IO0`–`IO7`. |
| `0x05` | `IO_EXTENSION_PWM_ADDR` | Write (1 byte) | Backlight PWM duty cycle (`0`–`255`, scaled from `0`–`97%`). |
| `0x06` | `IO_EXTENSION_ADC_ADDR` | Read (2 bytes) | 12-bit Battery ADC reading. |

### 3.2 IO Extension Pin Assignments

| Expander Pin | Bit | Function | Active State / Notes |
| :--- | :--- | :--- | :--- |
| `IO_EXTENSION_IO_0` | Bit 0 (`0x01`) | Expansion IO0 | General purpose |
| `IO_EXTENSION_IO_1` | Bit 1 (`0x02`) | **Touch Reset (`TP_RST`)** | `0` = Reset active, `1` = Normal operation |
| `IO_EXTENSION_IO_2` | Bit 2 (`0x04`) | **LCD Backlight (`LCD_BL`)** | `1` = Backlight ON, `0` = Backlight OFF |
| `IO_EXTENSION_IO_3` | Bit 3 (`0x08`) | **LCD Reset (`LCD_RST`)** | `1` = Normal operation (pull high during init) |
| `IO_EXTENSION_IO_4` | Bit 4 (`0x10`) | **SD Card CS (`SD_CS`)** | Set `1` (High) before initializing SDMMC |
| `IO_EXTENSION_IO_5` | Bit 5 (`0x20`) | **USB / CAN Select (`MUX_SEL`)** | `0` = USB mode, `1` = CAN (TWAI) mode |
| `IO_EXTENSION_IO_6` | Bit 6 (`0x40`) | Expansion IO6 | General purpose |
| `IO_EXTENSION_IO_7` | Bit 7 (`0x80`) | Expansion IO7 | General purpose |

---

## 4. RGB565 LCD Interface (`1024 × 600` — ST7262)

### 4.1 Control & Sync Signals

| LCD Signal | Macro Name | ESP32-S3 GPIO |
| :--- | :--- | :--- |
| `VSYNC` | `LCD_IO_RGB_VSYNC` | `GPIO_NUM_3` |
| `HSYNC` | `LCD_IO_RGB_HSYNC` | `GPIO_NUM_46` |
| `DE` | `LCD_IO_RGB_DE` | `GPIO_NUM_5` |
| `PCLK` | `LCD_IO_RGB_PCLK` | `GPIO_NUM_7` |
| `DISP` | `LCD_IO_RGB_DISP` | `-1` (Not connected) |

### 4.2 16-Bit RGB565 Data Bus (`data_gpio_nums[0..15]`)

| Data Index | LCD Color Bit | ESP32-S3 GPIO |
| :--- | :--- | :--- |
| `DATA0` | **B3** (Blue LSB) | `GPIO_NUM_14` |
| `DATA1` | **B4** | `GPIO_NUM_38` |
| `DATA2` | **B5** | `GPIO_NUM_18` |
| `DATA3` | **B6** | `GPIO_NUM_17` |
| `DATA4` | **B7** (Blue MSB) | `GPIO_NUM_10` |
| `DATA5` | **G2** (Green LSB) | `GPIO_NUM_39` |
| `DATA6` | **G3** | `GPIO_NUM_0` |
| `DATA7` | **G4** | `GPIO_NUM_45` |
| `DATA8` | **G5** | `GPIO_NUM_48` |
| `DATA9` | **G6** | `GPIO_NUM_47` |
| `DATA10` | **G7** (Green MSB) | `GPIO_NUM_21` |
| `DATA11` | **R3** (Red LSB) | `GPIO_NUM_1` |
| `DATA12` | **R4** | `GPIO_NUM_2` |
| `DATA13` | **R5** | `GPIO_NUM_42` |
| `DATA14` | **R6** | `GPIO_NUM_41` |
| `DATA15` | **R7** (Red MSB) | `GPIO_NUM_40` |

### 4.3 Verified RGB Timing Parameters

| Parameter | Value | Description |
| :--- | :--- | :--- |
| `h_res` | `1024` | Horizontal active pixels |
| `v_res` | `600` | Vertical active lines |
| `pclk_hz` | `24000000` – `30850000` | Pixel clock (`24 MHz`–`30.85 MHz`) |
| `hsync_pulse_width` | `162` | Horizontal sync pulse width (clocks) |
| `hsync_back_porch` | `152` | Horizontal back porch (clocks) |
| `hsync_front_porch` | `48` | Horizontal front porch (clocks) |
| `vsync_pulse_width` | `45` | Vertical sync pulse width (lines) |
| `vsync_back_porch` | `13` | Vertical back porch (lines) |
| `vsync_front_porch` | `3` | Vertical front porch (lines) |
| `pclk_active_neg` | `1` | Pixel clock active on falling edge |

With `Total H = 1024 + 162 + 152 + 48 = 1386` and `Total V = 600 + 45 + 13 + 3 = 661` (`916,146` clocks/frame):
- At `pclk_hz = 30.85 MHz`: Panel hardware refresh rate $\approx 33.67\text{ Hz}$
- At `pclk_hz = 26.00 MHz`: Panel hardware refresh rate $\approx 28.38\text{ Hz}$
- At `pclk_hz = 24.00 MHz`: Panel hardware refresh rate $\approx 26.20\text{ Hz}$

---

## 5. Capacitive Touch Controller (GT911)

| Signal | Pin / Control | Notes |
| :--- | :--- | :--- |
| `I2C_SDA` | `GPIO_NUM_8` | Shared `I2C_NUM_0` |
| `I2C_SCL` | `GPIO_NUM_9` | Shared `I2C_NUM_0` |
| `TOUCH_INT` | `GPIO_NUM_4` | Driven low during reset sequence to select I2C address `0x5D` |
| `TOUCH_RST` | `IO_EXTENSION_IO_1` | Reset sequence: `IO1=0` (100ms) -> `GPIO4=0` (100ms) -> `IO1=1` (200ms) |

---

## 6. Other Onboard Peripherals

| Peripheral | Signal | Pin / Expander | Notes |
| :--- | :--- | :--- | :--- |
| **RS485 (`UART1`)** | `TXD` / `RXD` | `GPIO_NUM_16` / `GPIO_NUM_15` | Automatic TX/RX direction control |
| **CAN (`TWAI`)** | `TX` / `RX` | `GPIO_NUM_20` / `GPIO_NUM_19` | Requires `IO_EXTENSION_IO_5 = 1` |
| **TF / SD Card** | `CLK` / `CMD` / `D0` | `GPIO12` / `GPIO11` / `GPIO13` | 1-bit SDMMC; requires `IO_EXTENSION_IO_4 = 1` |
| **Sensor / GPIO Header** | `GPIO6` | `GPIO_NUM_6` | Used for external LED / sensor examples |
