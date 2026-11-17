// Animated circles with framebuffer blur.
#include <VGA_esp32.h>
#include <math.h>

VGA_esp32 vga;
bool ready = false;

const uint16_t colors[] = { 0xF800, 0x07E0, 0x001F };
const float factors[3][2] = { {1.0f, 1.1f}, {0.9f, 1.02f}, {1.1f, 0.8f} };

void setup() {
    Serial.begin(115200);
    ready = vga.init(Mode320x240, BPP_16, false, true);
    if (!ready) {
        Serial.println("VGA init failed.");
        return;
    }
    vga.cls(0);
    vga.swap();
    vga.cls(0);
}

void loop() {
    if (!ready) return;

    const float p = millis() * 0.002f;
    const int radiusX = vga.Width() / 3;
    const int radiusY = vga.Height() / 3;

    for (int i = 0; i < 3; ++i) {
        int x = vga.CX() + (int)(sinf(p * factors[i][0]) * radiusX);
        int y = vga.CY() + (int)(cosf(p * factors[i][1]) * radiusY);
        vga.circle(x, y, 25, colors[i]);
    }

    vga.blur();
    vga.swap();
}
