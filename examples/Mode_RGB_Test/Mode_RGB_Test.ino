// RGB565 color test using the current VGA_esp32 API.
// Requires ESP32-S3 with PSRAM and a proper RGB565 VGA output circuit.
#include <VGA_esp32.h>

VGA_esp32 vga;

void setup() {
    Serial.begin(115200);
    if (!vga.init(Mode320x240, BPP_16, false, true)) {
        Serial.println("VGA init failed.");
        return;
    }
    vga.testRGB();
    vga.swap();
}

void loop() {}
