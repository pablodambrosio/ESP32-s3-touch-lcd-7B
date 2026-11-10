# Agent Instructions (AGENTS.md)

This repository is an **agentic, specification-driven ESP-IDF firmware project** targeting the **Waveshare ESP32-S3-Touch-LCD-7B** (`ESP32-S3-WROOM-1-N16R8`, 1024×600 RGB565 LCD).

All AI coding agents working in this repository MUST follow these rules.

---

## 1. Core Principles

1. **Spec-First Execution**:
   - Never implement features ad-hoc. Always read the active specification in [spec/](spec/README.md) first.
   - Verify every hardware pin and timing parameter against [doc/hardware-pinout.md](doc/hardware-pinout.md).
   - Follow system and memory constraints documented in [doc/architecture.md](doc/architecture.md).

2. **Hardware Safety & Accuracy**:
   - The target MCU is `esp32s3` with **16 MB Quad Flash** and **8 MB Octal PSRAM**.
   - **Never use GPIO26–GPIO37**: Those pins are internally wired to the Octal PSRAM and SPI Flash inside the `ESP32-S3-WROOM-1-N16R8` module.
   - The LCD backlight (`IO2`), LCD reset (`IO3`), Touch reset (`IO1`), SD CS (`IO4`), and USB/CAN mux (`IO5`) are controlled via the onboard **I2C IO Extension chip at slave address `0x24`** on `I2C_NUM_0` (`SDA = GPIO8`, `SCL = GPIO9`). They are **not** direct ESP32-S3 GPIOs.

3. **Non-Destructive Operations**:
   - Never run destructive git commands (`git reset --hard`, `git clean -fd`, deleting branches) or delete user files without explicit permission.
   - When asked an informational question, answer it directly without modifying files unless asked to act.

---

## 2. Build & Tooling Guidelines (VS Code ESP-IDF)

- **Target SoC**: `esp32s3`
- **Preferred Build Tool**: Always use the VS Code ESP-IDF extension tools (`espIdfCommands`) for building, flashing, monitoring, and size analysis instead of raw terminal shell commands.
- **Error Checking**: Use ` ESP_ERROR_CHECK()` or `ESP_RETURN_ON_ERROR()` on all ESP-IDF driver calls (`i2c_new_master_bus`, `esp_lcd_new_rgb_panel`, etc.).
- **Logging**: Use `ESP_LOGI`, `ESP_LOGW`, and `ESP_LOGE` with descriptive module tags (e.g., `"bsp_i2c"`, `"bsp_lcd"`, `"bringup"`).

---

## 3. Critical Display & PSRAM Configuration (`1024 × 600` RGB565)

Driving a `1024 × 600` 16-bit RGB panel requires ~1.23 MB per framebuffer (`1024 * 600 * 2 = 1,228,800` bytes). To achieve the target **25–30 FPS** without screen drift, tearing, or DMA underruns:

1. **Octal PSRAM @ 80 MHz**:
   - `CONFIG_SPIRAM=y`
   - `CONFIG_SPIRAM_MODE_OCT=y`
   - `CONFIG_SPIRAM_SPEED_80M=y`
   - `CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y`
   - `CONFIG_SPIRAM_RODATA=y`
2. **RGB LCD DMA & Bounce Buffers**:
   - Allocate framebuffers in PSRAM (`fb_in_psram = 1`).
   - Configure an internal SRAM bounce buffer (`bounce_buffer_size_px = 1024 * 20`) so GDMA feeds the LCD peripheral from fast internal SRAM rather than competing directly with CPU PSRAM writes.
   - Enable `CONFIG_ESP_LCD_RGB_ISR_IRAM_SAFE=y` and `CONFIG_ESP_LCD_RGB_RESTART_IN_VSYNC=y`.
3. **Pixel Clock & Frame Rate**:
   - Use `pclk_hz` between `24 MHz` and `30 MHz` (`pclk_active_neg = 1`).
   - Synchronize frame updates with VSYNC callbacks (`on_vsync`) to achieve smooth **25–30 FPS** updates.

---

## 4. Directory Conventions

- [doc/](doc/README.md): Reference documentation (hardware pinout, schematics summary, software architecture). Keep updated if hardware discoveries or architectural changes occur.
- [spec/](spec/README.md): Numbered feature specifications (`001-board-bringup.md`, `002-...`). Each spec defines Objective, Scope, Hardware/Kconfig Requirements, Implementation Plan, and Acceptance Criteria.
- [components/](components/README.md): Reusable ESP-IDF components (e.g., `bsp_waveshare_7b` or `io_extension` + `rgb_lcd_port`).
- [main/](main/main.c): Application entry point and high-level state machine.
