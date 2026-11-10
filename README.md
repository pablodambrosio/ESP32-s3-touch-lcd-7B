# AI Helper — Waveshare ESP32-S3-Touch-LCD-7B

An agentic, specification-driven embedded firmware project for the **[Waveshare ESP32-S3-Touch-LCD-7B](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B)** development board built on **ESP-IDF** and **FreeRTOS**.

---

## Hardware Overview

| Feature | Specification |
| :--- | :--- |
| **Board** | Waveshare ESP32-S3-Touch-LCD-7B |
| **MCU Module** | ESP32-S3-WROOM-1-N16R8 (Xtensa® 32-bit LX7 dual-core @ 240 MHz) |
| **Memory** | 512 KB Internal SRAM + **8 MB Octal PSRAM** + **16 MB Quad Flash** |
| **Display** | 7-inch IPS RGB LCD, **1024 × 600** resolution, RGB565 (65K colors), ST7262 driver |
| **Touch** | 5-point Capacitive Touch via **GT911** (I2C) |
| **IO Expander** | Custom I2C IO Extension MCU (`0x24`) for LCD Reset, Backlight PWM, Touch Reset, SD CS, CAN/USB Mux |
| **Peripherals** | CAN (TWAI), RS485, TF/SD Card (SDMMC 1-bit), I2C, UART, Full-Speed USB, Li-ion Battery ADC |

See [doc/hardware-pinout.md](doc/hardware-pinout.md) for the complete pin mapping and RGB timing specification.

---

## Repository Structure

```text
.
├── AGENTS.md                        # Agentic workflow instructions, constraints & conventions
├── CMakeLists.txt                   # Root ESP-IDF CMake configuration
├── README.md                        # Project overview and quick start
├── .github/
│   └── copilot-instructions.md      # Workspace instructions for AI coding agents
├── doc/                             # Hardware, pinout, and system architecture documentation
│   ├── README.md                    # Documentation index
│   ├── hardware-pinout.md           # Verified GPIO pinout, I2C expander map & LCD timings
│   └── architecture.md              # Display pipeline, PSRAM DMA & FreeRTOS architecture
├── spec/                            # Feature & milestone specifications (Spec-Driven Development)
│   ├── README.md                    # Specification index & lifecycle guide
│   └── 001-board-bringup.md         # Spec 001: Board bring-up, 25-30 FPS background sweep & test bars
├── components/                      # Modular ESP-IDF BSP and driver components
│   └── README.md                    # Component layout guidelines
└── main/                            # Application entry point
    ├── CMakeLists.txt
    └── main.c
```

---

## Agentic Spec-Driven Workflow

This project follows a strict **Specification-Driven Development (SDD)** loop:

1. **Read Context**: Consult [AGENTS.md](AGENTS.md), [doc/hardware-pinout.md](doc/hardware-pinout.md), and [doc/architecture.md](doc/architecture.md).
2. **Select Spec**: Pick the active specification in [spec/](spec/README.md) (starting with [spec/001-board-bringup.md](spec/001-board-bringup.md)).
3. **Implement**: Write modular ESP-IDF C code adhering to the hardware pinout and Kconfig requirements defined in the spec.
4. **Build & Verify**: Compile using the ESP-IDF build tools, resolve all diagnostics, flash to the board, and verify against the spec's Acceptance Criteria.
5. **Update Status**: Mark completed tasks and update the spec status in [spec/README.md](spec/README.md).

---

## Current Milestone

- **[Spec 001: Board Bring-Up & Display Diagnostics](spec/001-board-bringup.md)**
  - Bring up 8 MB Octal PSRAM, I2C0 (`GPIO8`/`GPIO9`), IO Extension (`0x24`), and the `1024 × 600` RGB565 LCD panel.
  - Achieve a stable **25–30 FPS** rendering loop using PSRAM framebuffers + internal SRAM bounce buffers.
  - Execute a full-screen background color check sequence and finalize on a steady-state 8-bar vertical color test pattern.
