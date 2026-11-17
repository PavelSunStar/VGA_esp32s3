// Animated 3D torus using the current VGA_esp32 drawing API.
// Original algorithm inspiration: https://github.com/MinhasKamal/CreepyCodeCollection
#include <VGA_esp32.h>
#include <math.h>
#include <string.h>

VGA_esp32 vga;
bool ready = false;

constexpr int cols = 80, rows = 40;
constexpr int STEPS_I = 314, STEPS_J = 90;
float zbuf[cols * rows];
uint8_t cbuf[cols * rows];
float sin_i[STEPS_I], cos_i[STEPS_I];
float sin_j[STEPS_J], cos_j[STEPS_J];
float A = 0.0f, B = 0.0f;

void initTables() {
    for (int k = 0; k < STEPS_I; ++k) {
        float a = k * 0.02f;
        sin_i[k] = sinf(a);
        cos_i[k] = cosf(a);
    }
    for (int k = 0; k < STEPS_J; ++k) {
        float a = k * 0.07f;
        sin_j[k] = sinf(a);
        cos_j[k] = cosf(a);
    }
}

void setup() {
    Serial.begin(115200);
    ready = vga.init(Mode320x240, BPP_16, false, true);
    if (!ready) {
        Serial.println("VGA init failed.");
        return;
    }
    initTables();
}

void loop() {
    if (!ready) return;

    memset(zbuf, 0, sizeof(zbuf));
    memset(cbuf, 0, sizeof(cbuf));

    const float e = sinf(A), g = cosf(A);
    const float m = cosf(B), n = sinf(B);

    for (int jj = 0; jj < STEPS_J; ++jj) {
        const float d = cos_j[jj], f = sin_j[jj];
        for (int ii = 0; ii < STEPS_I; ++ii) {
            const float c = sin_i[ii], l = cos_i[ii];
            const float h = d + 2.0f;
            const float D = 1.0f / (c * h * e + f * g + 5.0f);
            const float t = c * h * g - f * e;

            const int x = (int)(cols / 2 + (cols / 2) * D * (l * h * m - t * n));
            const int y = (int)(rows / 2 + (rows / 2) * D * (l * h * n + t * m));
            if (x < 0 || x >= cols || y < 0 || y >= rows) continue;

            const int o = x + cols * y;
            int N = (int)(8.0f * ((f * e - c * d * g) * m
                                 - c * d * e - f * g - l * d * n));
            if (N < 0) N = 0;
            if (N > 31) N = 31;
            if (D > zbuf[o]) {
                zbuf[o] = D;
                cbuf[o] = (uint8_t)N;
            }
        }
    }

    vga.cls(0);
    const int cellW = vga.Width() / cols;
    const int cellH = vga.Height() / rows;
    const int originX = (vga.Width() - cellW * cols) / 2;
    const int originY = (vga.Height() - cellH * rows) / 2;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            int shade = cbuf[x + y * cols];
            if (shade) {
                uint16_t color = (uint16_t)(shade << 11);
                vga.fillRect(originX + x * cellW, originY + y * cellH,
                             originX + (x + 1) * cellW - 1,
                             originY + (y + 1) * cellH - 1, color);
            }
        }
    }
    vga.swap();

    A += 0.04f;
    B += 0.02f;
}
