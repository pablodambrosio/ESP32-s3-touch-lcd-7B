---
description: Workspace instructions for Waveshare ESP32-S3-Touch-LCD-7B ESP-IDF firmware development
---

# Copilot Instructions — Waveshare ESP32-S3-Touch-LCD-7B

1. **Follow `AGENTS.md`**: Always read [AGENTS.md](../AGENTS.md) for project rules, build tool preferences, and safety constraints.
2. **Verify Hardware Pinout**: Always check [doc/hardware-pinout.md](../doc/hardware-pinout.md) before configuring any GPIO, I2C device, or RGB LCD timing parameter.
3. **Spec-Driven Development**: Check [spec/README.md](../spec/README.md) and the active spec (currently [spec/001-board-bringup.md](../spec/001-board-bringup.md)) before writing or modifying firmware code.
4. **ESP-IDF Version & APIs**:
   - Use modern ESP-IDF driver APIs (`driver/i2c_master.h` instead of legacy `driver/i2c.h`, `esp_lcd_panel_rgb.h`, `esp_lcd_panel_ops.h`).
   - Use the VS Code ESP-IDF extension tools for building, flashing, and monitoring.
