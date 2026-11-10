# Components (`components/`)

This directory holds modular ESP-IDF components for the **Waveshare ESP32-S3-Touch-LCD-7B**.

## Planned Components

| Component | Responsibility |
| :--- | :--- |
| `bsp_waveshare_7b` | Board Support Package: I2C0 master bus (`GPIO8`/`GPIO9`), IO Extension IC (`0x24`) driver, and ST7262 `1024 × 600` RGB565 LCD panel driver with VSYNC synchronization. |
| `bsp_touch_gt911` | *(Future Spec 002)* GT911 capacitive touch controller driver over I2C0. |
| `ui_lvgl_port` | *(Future Spec 003)* LVGL display/input adapter synchronized with RGB VSYNC. |
