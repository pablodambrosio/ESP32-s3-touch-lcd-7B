# Project Documentation (`doc/`)

This directory contains authoritative hardware and software architecture documentation for the **Waveshare ESP32-S3-Touch-LCD-7B** project.

## Contents

| Document | Description |
| :--- | :--- |
| [hardware-pinout.md](hardware-pinout.md) | Complete GPIO assignment table, I2C IO Extension (`0x24`) register & pin map, RGB565 LCD (`1024 × 600`) timing parameters, and peripheral pins. |
| [architecture.md](architecture.md) | System architecture, PSRAM + SRAM bounce-buffer display pipeline, frame rate budgeting (target 25–30 FPS), and FreeRTOS task organization. |

## Official External References

- [Waveshare ESP32-S3-Touch-LCD-7B Wiki](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B)
- [Waveshare ESP-IDF Guide & Examples](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B/ESP-IDF)
- [ESP32-S3-Touch-LCD-7B Schematic (PDF)](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B/ESP32-S3-Touch-LCD-7B-Schematic.pdf)
- [Waveshare Official GitHub Repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7B)
