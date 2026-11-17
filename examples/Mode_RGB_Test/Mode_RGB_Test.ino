
#include <Arduino.h>
#include <VGA_esp32.h>

VGA_esp32 vga;

// ==========================================
// SETUP
// ==========================================

void setup() {

    Serial.begin(115200);

    // 640x480
    // RGB565
    // Палитра OFF
    // Один framebuffer
    bool ready = vga.init(
        Mode640x480,
        BPP_16,
        false,
        false
    );

    vga.testRGB();

    Serial.printf(
        "RGB TEST OK | %dx%d | 16-bit\n",
        vga.Width(),
        vga.Height()
    );
}

void loop() {
    // Статическое изображение
}
