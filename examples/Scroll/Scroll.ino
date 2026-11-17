// Scrolling *pattern* demo for the current VGA_esp32 API.
// The old scrollX/scrollY/window-scroll methods are not present in this API.
// This example redraws a shifted tile pattern rather than scrolling memory.
#include <VGA_esp32.h>

VGA_esp32 vga;
bool ready = false;
int offsetX = 0;
int offsetY = 0;

void setup() {
    Serial.begin(115200);
    ready = vga.init(Mode320x240, BPP_16, false, true);
    if (!ready) Serial.println("VGA init failed.");
}

void loop() {
    if (!ready) return;

    constexpr int tile = 32;
    const int w = vga.Width();
    const int h = vga.Height();

    vga.cls(0x0841);
    for (int y = -tile; y < h + tile; y += tile) {
        for (int x = -tile; x < w + tile; x += tile) {
            int px = x + offsetX;
            int py = y + offsetY;
            int parity = ((x / tile) + (y / tile)) & 1;
            uint16_t col = parity ? 0x18C3 : 0x39E7;
            vga.fillRect(px, py, px + tile - 2, py + tile - 2, col);
        }
    }
    vga.putString(8, 8, "Pattern scroll", 0xFFFF);
    vga.swap();

    offsetX = (offsetX + 1) % tile;
    offsetY = (offsetY + 1) % tile;
}
