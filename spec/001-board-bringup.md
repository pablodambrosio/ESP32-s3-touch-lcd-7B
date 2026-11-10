# Spec 001: Board Bring-Up & Display Diagnostic Sequence

- **Status**: `IN_PROGRESS` (Implemented & Built — Ready for Hardware Verification)
- **Target Board**: Waveshare ESP32-S3-Touch-LCD-7B (`ESP32-S3-WROOM-1-N16R8`)
- **Display**: 7-inch 1024 × 600 RGB565 LCD (ST7262)
- **Target Frame Rate**: **25–30 FPS** (`33.3 ms` – `40.0 ms` per frame)

---

## 1. Objective

Bring up the core hardware of the **Waveshare ESP32-S3-Touch-LCD-7B** from a blank ESP-IDF project and validate display signal integrity, PSRAM DMA stability, and frame rate performance.

Upon boot, the firmware must:
1. Initialize the `I2C_NUM_0` bus (`GPIO8` SDA, `GPIO9` SCL) and the onboard **IO Extension IC (`0x24`)**.
2. Initialize the **1024 × 600 16-bit RGB565 LCD** using Octal PSRAM framebuffers and internal SRAM bounce buffers tuned for **25–30 FPS**.
3. Turn on the LCD backlight (`IO_EXTENSION_IO_2 = 1`) only after the initial framebuffer is cleared to prevent power-on white flash/garbage.
4. Execute a **Background Color Check Sequence** (smooth animated color transitions and solid primary/secondary color fills at **25–30 FPS** while logging real-time FPS to UART).
5. Finalize and hold on a **Full-Screen 8-Bar Vertical Color Test Pattern** (`1024 × 600`) to visually verify color accuracy, bit alignment across all 16 RGB data lines, and zero horizontal drift.

---

## 2. Hardware & Pin Configuration

All pins must strictly match [doc/hardware-pinout.md](../doc/hardware-pinout.md):

- **I2C0 Master**: `SDA = GPIO8`, `SCL = GPIO9`, `400 kHz`
- **IO Extension (`0x24`)**:
  - Mode Register `0x02` = `0xFF` (all pins output)
  - `IO_EXTENSION_IO_2` (Bit 2) = LCD Backlight (`1` = ON, `0` = OFF)
  - `IO_EXTENSION_IO_3` (Bit 3) = LCD Reset (`1` = Normal)
- **RGB565 LCD (`1024 × 600`)**:
  - `VSYNC = GPIO3`, `HSYNC = GPIO46`, `DE = GPIO5`, `PCLK = GPIO7`, `DISP = -1`
  - `DATA0..4` (Blue B3..B7): `GPIO14`, `GPIO38`, `GPIO18`, `GPIO17`, `GPIO10`
  - `DATA5..10` (Green G2..G7): `GPIO39`, `GPIO0`, `GPIO45`, `GPIO48`, `GPIO47`, `GPIO21`
  - `DATA11..15` (Red R3..R7): `GPIO1`, `GPIO2`, `GPIO42`, `GPIO41`, `GPIO40`
- **RGB Timing**:
  - `pclk_hz = 26 * 1000 * 1000` (26 MHz $\rightarrow \approx 28.4\text{ FPS}$ native VSYNC refresh, within the 25–30 FPS target)
  - `hsync_pulse_width = 162`, `hsync_back_porch = 152`, `hsync_front_porch = 48`
  - `vsync_pulse_width = 45`, `vsync_back_porch = 13`, `vsync_front_porch = 3`
  - `pclk_active_neg = 1`

---

## 3. Required ESP-IDF Configuration (`sdkconfig.defaults`)

To support 8 MB Octal PSRAM and high-throughput RGB DMA without underruns:

```ini
# Target & Flash
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# CPU Frequency
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# 8MB Octal PSRAM @ 80MHz
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y
CONFIG_SPIRAM_RODATA=y

# Cache & Memory
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y

# RGB LCD Driver Stability
CONFIG_LCD_RGB_ISR_IRAM_SAFE=y
CONFIG_LCD_RGB_RESTART_IN_VSYNC=y

# FreeRTOS Tick Rate (1000 Hz for accurate frame pacing)
CONFIG_FREERTOS_HZ=1000

# Compiler Optimization for Performance
CONFIG_COMPILER_OPTIMIZATION_PERF=y
```

---

## 4. Functional Requirements & Visual Sequence

### 4.1 Phase 1: Background Color Check (Target 25–30 FPS)

Run a timed diagnostic sequence (~8–10 seconds total) paced at **25–30 FPS** (synchronized to VSYNC or a `35 ms` frame timer):

1. **Primary & Secondary Channel Verification** (1.0 s each):
   - Pure Red (`0xF800`) — verifies `R3..R7` (`DATA11..15`)
   - Pure Green (`0x07E0`) — verifies `G2..G7` (`DATA5..10`)
   - Pure Blue (`0x001F`) — verifies `B3..B7` (`DATA0..4`)
   - Pure White (`0xFFFF`) — verifies all 16 data bits high simultaneously
   - Pure Black (`0x0000`) — verifies all 16 data bits low & backlight uniformity
2. **Smooth Background Color Sweep** (~4.0 s, 120 frames @ ~28–30 FPS):
   - Continuously interpolate a pleasing background color gradient/hue across the entire `1024 × 600` screen every frame.
   - Measure actual frame render + transfer time using `esp_timer_get_time()` and log the achieved FPS every 30 frames:
     `I (xxxx) bringup: Background sweep FPS: 28.4 (frame time: 35.2 ms)`

### 4.2 Phase 2: Finalization with Color Test Bars

After the background color check completes, render a crisp **8-Bar Vertical Color Test Pattern** across the full `1024 × 600` screen (`128` pixels wide per bar) and hold it indefinitely:

| Bar Index | X Range (`px`) | Color Name | RGB888 | RGB565 Hex |
| :--- | :--- | :--- | :--- | :--- |
| 0 | `0 .. 127` | **White** | `(255, 255, 255)` | `0xFFFF` |
| 1 | `128 .. 255` | **Yellow** | `(255, 255, 0)` | `0xFFE0` |
| 2 | `256 .. 383` | **Cyan** | `(0, 255, 255)` | `0x07FF` |
| 3 | `384 .. 511` | **Green** | `(0, 255, 0)` | `0x07E0` |
| 4 | `512 .. 639` | **Magenta** | `(255, 0, 255)` | `0xF81F` |
| 5 | `640 .. 767` | **Red** | `(255, 0, 0)` | `0xF800` |
| 6 | `768 .. 895` | **Blue** | `(0, 0, 255)` | `0x001F` |
| 7 | `896 .. 1023` | **Black** | `(0, 0, 0)` | `0x0000` |

Optional border/grid accent: A 2-pixel white/gray border frame around the outer edge (`1024 × 600`) to confirm exact horizontal and vertical porch alignment (no cropped edges).

---

## 5. Implementation Plan

- [x] **Step 1**: Create `sdkconfig.defaults` with 16MB Flash, 8MB Octal PSRAM @ 80MHz, and RGB LCD IRAM-safe settings.
- [x] **Step 2**: Implement `components/bsp_waveshare_7b`:
  - `bsp_io_extension.c/.h`: I2C0 initialization (`GPIO8`/`GPIO9`) and slave `0x24` register driver (`Mode`, `Output`, `PWM`).
  - `bsp_rgb_lcd.c/.h`: RGB565 panel initialization (`1024 × 600`), VSYNC semaphore notification callback, double-buffered PSRAM + SRAM bounce buffer (`1024 * 20` px) setup, and fast 32-bit aligned fill & test-bar rendering routines.
- [x] **Step 3**: Implement the bring-up state machine in `main/main.c` executing Phase 1 (Background Color Check @ 25–30 FPS with FPS telemetry) followed by Phase 2 (Final 8-Bar Test Pattern).
- [ ] **Step 4**: Flash and monitor serial output on hardware to verify 25–30 FPS performance and visual output.

---

## 6. Acceptance Criteria

1. **Zero Build Warnings/Errors**: Project compiles cleanly for `esp32s3` under ESP-IDF.
2. **PSRAM & I2C Bring-Up**: Serial log confirms 8 MB Octal PSRAM initialized and I2C communication with `0x24` succeeds (`ESP_OK`).
3. **Frame Rate Target Met**: Logged frame rate during full-screen background color updates is consistently between **25 FPS and 30 FPS** (`33.3 ms` – `40.0 ms` per frame).
4. **Visual Integrity**:
   - No white screen flash before framebuffer initialization.
   - Primary colors (R, G, B, White, Black) and smooth background color sweep render without horizontal tearing or drift.
   - Final 8-bar test pattern fills all `1024 × 600` pixels with sharp vertical boundaries.
