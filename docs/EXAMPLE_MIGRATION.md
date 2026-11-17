# Example migration notes (2026-11-17)

This branch updates the **four existing Arduino examples** against the interface currently declared by `src/VGA_esp32.h`. It does **not** modify the library implementation, pins, VGA timings or buffer lifecycle.

## Changes

- `library.properties`: corrected `includes` to `VGA_esp32.h`.
- `Mode_RGB_Test`: uses the implemented `vga.testRGB()`.
- `Circles`: uses `VGA_esp32` methods and current `Width()/Height()/CX()/CY()` getters.
- `Doughnut_3d`: uses current VGA mode, `fillRect()`, and properly sized framebuffer coordinates.
- `Scroll`: **different behavior from old demo**. Legacy `scrollX/scrollY/winScroll*` operations are not declared in the present API, so the replacement demonstrates a moving tiled pattern by redrawing each frame; it does not imply framebuffer scrolling is implemented.

## Compile and hardware verification still needed

These changes were migrated by inspecting repository sources; **no successful Arduino IDE compilation or ESP32-S3 hardware test is claimed**. Before merging:

1. In Arduino IDE select your exact ESP32-S3 board, enable PSRAM and keep the previously working Arduino-ESP32 core version.
2. Compile each of the four examples separately.
3. Run each on your known-good VGA wiring and monitor.
4. Verify 320×240 RGB565 operation, `swap()`, and serial error output.
5. Compare visual output and monitor stability against your known-good local builds.
6. Resolve any compiler diagnostics in this branch; do not overwrite known-good local source.

### Important separate safety issue

In single-buffer mode the current implementation aliases `fb1 = fb0` but `destroy()` frees both. This is a potential double-free on destruction/reinitialization. It is intentionally outside this compatibility-only change and should be fixed/tested in a **separate** patch.

## Preservation

`main` is unchanged until the pull request is reviewed/merged. Make an offline backup of local uncommitted work and hardware/PCB design files before merging any new code.
