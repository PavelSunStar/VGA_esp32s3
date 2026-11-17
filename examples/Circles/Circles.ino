
#include <Arduino.h>
#include <VGA_esp32.h>
#include <math.h>

VGA_esp32 vga;

bool ready = false;

// Параметры шариков
int scrX3, scrY3;

// RGB565: красный, зелёный, синий
uint16_t colors[] = {
    0xF800,
    0x07E0,
    0x001F
};

float factors[][2] = {
    {1.0f, 1.1f},
    {0.9f, 1.02f},
    {1.1f, 0.8f}
};

uint32_t lastFPS = 0;

void setup() {
    Serial.begin(115200);

    // 320x240 -> VGA 640x480
    // 16-bit RGB565, без палитры, двойной буфер
    ready = vga.init(
        Mode320x240,
        BPP_16,
        false,
        false
    );

    if (!ready) {
        Serial.println("VGA INIT ERROR!");
        return;
    }

    scrX3 = vga.Width() / 3;
    scrY3 = vga.Height() / 3;

    // Очистка обоих буферов
    vga.cls(0);
    vga.swap();
    vga.cls(0);

    Serial.println("VGA Circles started!");
}

void loop() {
    if (!ready) return;

    float p = millis() * 0.002f;

    // Рисуем шарики
    for (int i = 0; i < 3; i++) {
        int x = vga.CX() + sinf(p * factors[i][0]) * scrX3;
        int y = vga.CY() + cosf(p * factors[i][1]) * scrY3;
        vga.circle(x, y, 25, colors[i]);
    }

    // Размытие для следов
    vga.blur();
}
