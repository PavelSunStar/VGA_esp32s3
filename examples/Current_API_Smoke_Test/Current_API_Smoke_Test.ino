// Minimal current-API VGA smoke test for ESP32-S3 with PSRAM.
// Source-derived example; not yet verified by CI or on hardware.
#include <VGA_esp32.h>

VGA_esp32 vga;
bool ready = false;

void setup() {
    Serial.begin(115200);
    ready = vga.init(Mode320x240, BPP_16, false, true);
    if (!ready) {
        Serial.println("VGA init failed: check PSRAM, pins and monitor.");
        return;
    }
    vga.cls(0x0000);
    vga.fillRect(10, 10, 100, 60, 0xF800);
    vga.circle(160, 120, 40, 0x07E0);
    vga.putString(8, 220, "VGA_esp32", 0xFFFF);
    vga.swap();
}

void loop() {}
