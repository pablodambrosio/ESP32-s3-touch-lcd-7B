# Specifications (`spec/`)

This directory tracks all specifications for the **Waveshare ESP32-S3-Touch-LCD-7B** firmware project.

## Specification Lifecycle

- `DRAFT` — Initial design under review.
- `READY` — Approved and ready for agentic implementation.
- `IN_PROGRESS` — Currently being implemented and tested.
- `VERIFIED` — Built, flashed, and verified against all acceptance criteria.

## Active & Planned Specifications

| ID | Specification | Target | Status |
| :--- | :--- | :--- | :--- |
| **001** | [Board Bring-Up & Display Diagnostics](001-board-bringup.md) | 25–30 FPS RGB565 bring-up, background color sweep, and final SMPTE/8-bar test pattern | `IN_PROGRESS` |
| **002** | *Capacitive Touch (GT911) Bring-Up (Planned)* | I2C touch initialization, interrupt handling, and 5-point coordinate readout | `PLANNED` |
| **003** | *LVGL Integration & UI Shell (Planned)* | Tear-free LVGL v8/v9 port synced with RGB VSYNC at 25–30 FPS | `PLANNED` |
