# VGA_esp32s3

Experimental **ESP32-S3 VGA graphics library for Arduino**, built around the ESP-IDF RGB LCD peripheral (`esp_lcd_rgb_panel`) and external PSRAM framebuffers.

> **Project status:** Active development / experimental. This README documents the code currently present in the repository, not every feature tested in local development. The existing examples appear to target an older API; see [Compatibility notes](#compatibility-notes).

## Features in the current source

- RGB565 (16-bit) and 8-bit framebuffer modes; optional RGB332-indexed palette output converted to RGB565 during the bounce-buffer callback.
- PSRAM framebuffer allocation, optional double buffering and `swap()`.
- VGA timing output currently based on 640×480 timing, with software scaling for lower logical resolutions.
- Pixels, lines, rectangles, circles, ellipses, polygons, stars, arcs, flood fill and blur.
- 8×8 default font; text cursor, formatted text and affine text rendering.
- `Sprite` helper with image, mask, scaling and affine drawing operations.
- Background loading and scrolling/drawing helpers.
- Frame counter, FPS reporting, and frame-based triggers.

## Hardware

- **ESP32-S3 with external PSRAM** (developed with an N16R8-class board).
- A **suitable RGB565-to-analog-VGA interface**, properly designed for a 75 Ω VGA load, plus a VGA monitor. **Do not connect GPIO pins directly to a VGA monitor without the appropriate interface.**
- USB cable for programming and serial logs.

### Default VGA GPIO mapping

From `src/structures.h` (`defPins_S3`):

| Signal | GPIO |
|---|---|
| R0, R1, R2, R3, R4 | 15, 7, 6, 5, 4 |
| G0, G1, G2, G3, G4, G5 | 14, 13, 12, 11, 10, 9 |
| B0, B1, B2, B3, B4 | 3, 8, 18, 17, 16 |
| HSYNC, VSYNC | 1, 2 |

Pin numbers describe the firmware assignment, **not** a complete resistor-DAC schematic. Check your own board layout, VGA DAC resistors, voltage levels, grounding and connector wiring before powering the monitor.

## Installation

1. Install Arduino IDE and the Espressif **esp32** board package.
2. Select an ESP32-S3 board profile matching your hardware, and **enable PSRAM** in the board settings.
3. Download/clone this repository into your Arduino libraries directory under the folder name `VGA_esp32s3`, or use **Sketch → Include Library → Add .ZIP Library…**.
4. Restart Arduino IDE if the library does not appear.

The exact Arduino-ESP32 package version is **not pinned or CI-tested** in this repository. ESP-IDF RGB panel structures and callback interfaces can vary between core versions. If you have a working local build, record its board profile, Arduino-ESP32 version and PSRAM settings before upgrading.

## Minimal example (current API)

The current public header is `src/VGA_esp32.h`, and the class name is `VGA_esp32`:

```cpp
#include <VGA_esp32.h>

VGA_esp32 vga;

void setup() {
    Serial.begin(115200);

    if (!vga.init(Mode320x240, BPP_16, false, true)) {
        Serial.println("VGA initialization failed");
        return;
    }

    vga.cls(0x0000);
    vga.fillRect(20, 20, 140, 80, 0xF800); // red RGB565
    vga.circle(160, 120, 45, 0x07E0);      // green RGB565
    vga.putString(12, 12, "VGA ESP32-S3", 0xFFFF);
    vga.swap();
}

void loop() {}
```

This is an API example derived from the present header; **hardware execution and compilation are not verified in this documentation change**.

### `init()` parameters

```cpp
bool init(ScreenMode mode = Mode640x480,
          ColorDepth depth = BPP_8,
          bool usePal = false,
          bool doubleBuffer = false);
```

- `mode`: logical framebuffer size.
- `depth`: `BPP_8` or `BPP_16`.
- `usePal`: uses an 8-bit indexed framebuffer and a 256-entry RGB565 palette.
- `doubleBuffer`: allocates separate drawing and display buffers; call `swap()` after drawing.

### Modes actually implemented in `setMode()`

| Logical size | Output timing | Scale |
|---|---|---|
| 640×480 | 640×480 | 1× |
| 320×240 | 640×480 | 2× |
| 160×120 | 640×480 | 4× |

Additional `ScreenMode` enum members exist in `src/structures.h`, but the current `setMode()` implementation rejects them.

### Selected drawing API

```cpp
vga.cls(0);
vga.putPixel(x, y, color);
vga.line(x0, y0, x1, y1, color);
vga.rect(x0, y0, x1, y1, color);
vga.fillRect(x0, y0, x1, y1, color);
vga.circle(x, y, radius, color);
vga.fillCircle(x, y, radius, color);
vga.setTextSize(2);
vga.putString(x, y, "Hello", color);
vga.putAffineString(x, y, "ROTATE", color, angle, zoomX, zoomY);
vga.setPalCol(index, r, g, b);
vga.swap();
```

In RGB565 mode colors use 16-bit packed RGB565 values (e.g. red `0xF800`, green `0x07E0`, blue `0x001F`). In 8-bit mode colors are byte values; in palette mode the byte indexes the live palette.

For additional methods, refer to [`src/VGA_esp32.h`](src/VGA_esp32.h) and [`src/Sprite.h`](src/Sprite.h).

## Directory layout

```text
VGA_esp32s3/
├── README.md
├── LICENSE
├── library.properties
├── examples/                # historical sketches; see compatibility note
└── src/
    ├── VGA_esp32.h/.cpp     # public VGA class and drawing pipeline
    ├── structures.h         # pins, modes, screen structures
    ├── colorConverter.h
    ├── Sprite.h/.cpp
    ├── Matrix.h/.cpp
    ├── fonts/
    └── include/             # drawing, font, palette, background, init code
```

## Compatibility notes

**The source and examples are currently out of sync.** The checked-in sketches in `examples/` use legacy headers/classes such as `VGA_esp32s3.h`, `VGA_GFX.h`, `VGA_esp32s3`, and methods such as `setPins()` / `ScrWidth()`. The present library instead exposes `VGA_esp32.h` and `VGA_esp32`. Those examples must be ported before they can be expected to compile against this commit.

Also, `library.properties` currently declares `includes=VGA_ESP32S3.h`, which does **not** match the checked-in public header. Manual `#include <VGA_esp32.h>` is the current interface; a separate compatibility/metadata fix should be tested before release.

**Known source-review concern:** `getMemory()` aliases `fb1` to `fb0` in single-buffer mode, while `destroy()` frees both pointers without a distinct-pointer check. Avoid repeated initialization/destruction in single-buffer mode until this is addressed and tested.

The runtime, electrical performance, all memory modes and compiler versions have **not** been independently validated by this documentation update. See [`docs/STATUS.md`](docs/STATUS.md) for a review checklist.

## Preserving the project

Git history is valuable but not a substitute for a separate backup. On Windows, download a ZIP snapshot from GitHub **and** store a second copy of your working Arduino library in another location. Keep tested working firmware/sketches, board-setting screenshots, any VGA PCB/Gerber files, and known-good Arduino-ESP32 versions together. Do not overwrite a working library folder with an untested GitHub version.

Suggested local commands if Git is installed:

```bash
git clone https://github.com/PavelSunStar/VGA_esp32s3.git
cd VGA_esp32s3
git log --oneline -10
git status
```

For substantial changes, create a separate branch and submit a pull request. Prefer version tags for tested releases and keep a hardware-backed smoke-test checklist.

## License

[MIT](LICENSE) — copyright information is in the existing license file.
