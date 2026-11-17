# Current repository status and preservation checklist

Reviewed against the default branch on **2026-11-17**. This is a **source review, not a compile or hardware test**.

## Confirmed in the checked-in source

- Public class: `VGA_esp32` in `src/VGA_esp32.h`.
- Three modes implemented in `VGA_esp32::setMode()`: 640×480, 320×240 and 160×120, all using 640×480 output timings.
- RGB565 and 8-bit framebuffer support, optional indexed palette and double-buffer operations.
- Drawing primitives, text, sprite and background code are included.
- `library.properties` contains the old `includes=VGA_ESP32S3.h` entry.
- The `examples/` sketches reference older API names and need porting.

## Items to verify before a stable release

- [ ] Compile a new current-API sketch on the intended Arduino-ESP32 core and ESP32-S3 board profile.
- [ ] Record Arduino IDE, board package and PSRAM configuration used for a successful build.
- [ ] Confirm VGA RGB565 DAC wiring, resistor ladder, grounding and sync polarity on the actual PCB.
- [ ] Smoke-test 640×480, 320×240 and 160×120.
- [ ] Smoke-test BPP_8, BPP_16, indexed palette and both single/double buffering modes.
- [ ] Review single-buffer destruction: `fb1 == fb0` can result in two frees.
- [ ] Review initialization failure paths, semaphore/panel lifetime and allocated memory cleanup.
- [ ] Port existing example sketches to the new header/class API; add a compile test.
- [ ] Update library metadata `includes=` after successful testing.
- [ ] Add CI once the toolchain version and supported hardware configuration are documented.
- [ ] Tag a tested revision and archive its complete source/PCB and build settings.

## Non-goals of this documentation-only change

No production code, VGA pin mapping, algorithms, timing or existing examples are changed here. Do not treat a documentation PR as a hardware-tested release.

## Backup recommendation

Maintain an offline copy of the working source and tested sketches (including any changes not yet pushed to GitHub). Export any PCB design sources, Gerbers and bill of materials independently. Store a second copy away from the development PC. GitHub itself should not be your only backup.
