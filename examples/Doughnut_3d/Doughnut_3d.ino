
#include <Arduino.h>
#include <VGA_esp32.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

VGA_esp32 vga;
bool ready = false;

// ==========================================
// НАСТРОЙКИ
// ==========================================

constexpr int COLS = 80;
constexpr int ROWS = 40;

// Меняй эти значения для качества/скорости.
// 1 = максимальная детализация
// 2 = быстрый режим
// 3 = очень быстрый режим
constexpr int STEP_I = 1;
constexpr int STEP_J = 1;

constexpr int STEPS_I = 314;
constexpr int STEPS_J = 90;

float zbuf[COLS * ROWS];
uint8_t cbuf[COLS * ROWS];

float sin_i[STEPS_I], cos_i[STEPS_I];
float sin_j[STEPS_J], cos_j[STEPS_J];

int scrW, scrH;
int cellW, cellH;
int offsetX, offsetY;

float A = 0.0f;
float B = 0.0f;

// ==========================================
// FPS И ПРОФИЛИРОВАНИЕ
// ==========================================

uint32_t fpsTimer = 0;
uint32_t fpsFrames = 0;

uint32_t drawFPS = 0;

uint64_t renderTimeSum = 0;
uint64_t swapTimeSum = 0;

// ==========================================
// ТАБЛИЦЫ SIN / COS
// ==========================================

void initTables() {

    for (int i = 0; i < STEPS_I; i++) {
        float a = i * 0.02f;
        sin_i[i] = sinf(a);
        cos_i[i] = cosf(a);
    }

    for (int j = 0; j < STEPS_J; j++) {
        float a = j * 0.07f;
        sin_j[j] = sinf(a);
        cos_j[j] = cosf(a);
    }
}

// ==========================================
// БЫСТРАЯ ОТРИСОВКА RGB565
// ==========================================

void drawTorus() {

    // Buf() указывает на буфер рисования.
    uint16_t* fb = (uint16_t*)vga.Buf();

    if (!fb) return;

    // Быстрая очистка всего кадра.
    memset(
        fb,
        0,
        (size_t)scrW * scrH * sizeof(uint16_t)
    );

    // Рисуем только видимые ячейки.
    for (int y = 0; y < ROWS; y++) {

        int py = offsetY + y * cellH;

        for (int x = 0; x < COLS; x++) {

            uint8_t brightness =
                cbuf[y * COLS + x];

            if (!brightness) continue;

            // RGB565 RED
            uint16_t color =
                (uint16_t)brightness << 11;

            int px = offsetX + x * cellW;

            // Четыре одинаковых RGB565 пикселя
            // в одном 64-битном значении.
            uint64_t packed =
                (uint64_t)color |
                ((uint64_t)color << 16) |
                ((uint64_t)color << 32) |
                ((uint64_t)color << 48);

            // Для 320x240:
            // одна ячейка = 4x6 пикселей.
            if (cellW == 4) {

                for (int yy = 0; yy < cellH; yy++) {

                    uint16_t* dst =
                        fb + (py + yy) * scrW + px;

                    memcpy(dst, &packed, 8);
                }

            } else {

                // Универсальный вариант
                for (int yy = 0; yy < cellH; yy++) {

                    uint16_t* dst =
                        fb + (py + yy) * scrW + px;

                    for (int xx = 0; xx < cellW; xx++) {
                        dst[xx] = color;
                    }
                }
            }
        }
    }
}

// ==========================================
// БЫСТРЫЙ РАСЧЁТ 3D TORUS
// ==========================================

void renderTorus() {

    memset(zbuf, 0, sizeof(zbuf));
    memset(cbuf, 0, sizeof(cbuf));

    // Считаем только один раз за кадр
    const float sinA = sinf(A);
    const float cosA = cosf(A);
    const float sinB = sinf(B);
    const float cosB = cosf(B);

    const float halfW = COLS * 0.5f;
    const float halfH = ROWS * 0.5f;

    // ======================================
    // ВНЕШНИЙ ЦИКЛ
    // ======================================

    for (int jj = 0; jj < STEPS_J; jj += STEP_J) {

        const float d = cos_j[jj];
        const float f = sin_j[jj];

        const float h = d + 2.0f;

        // Частичные вычисления для всей строки
        const float fCosA = f * cosA;
        const float fSinA = f * sinA;
        const float dCosA = d * cosA;
        const float dSinA = d * sinA;

        // ==================================
        // ВНУТРЕННИЙ ЦИКЛ
        // ==================================

        for (int ii = 0; ii < STEPS_I; ii += STEP_I) {

            const float c = sin_i[ii];
            const float l = cos_i[ii];

            const float ch = c * h;

            const float D =
                1.0f / (ch * sinA + fCosA + 5.0f);

            const float t =
                ch * cosA - fSinA;

            const float lh = l * h;

            const float X =
                lh * cosB - t * sinB;

            const float Y =
                lh * sinB + t * cosB;

            const int x =
                (int)(halfW + halfW * D * X);

            const int y =
                (int)(halfH + halfH * D * Y);

            if ((unsigned)x >= COLS ||
                (unsigned)y >= ROWS)
                continue;

            const int o = y * COLS + x;

            // Невидимую точку не освещаем
            if (D <= zbuf[o]) continue;

            zbuf[o] = D;

            // Освещение поверхности
            const float light =
                (fSinA - c * dCosA) * cosB
                - c * dSinA
                - fCosA
                - l * d * sinB;

            int N = (int)(8.0f * light);

            if (N < 0) N = 0;
            if (N > 31) N = 31;

            cbuf[o] = (uint8_t)N;
        }
    }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

    Serial.begin(115200);

    // RGB565
    // 320x240 -> VGA 640x480
    // ДВА FRAMEBUFFER
    ready = vga.init(
        Mode320x240,
        BPP_16,
        false,
        true
    );

    if (!ready) {
        Serial.println("VGA INIT ERROR!");
        return;
    }

    scrW = vga.Width();
    scrH = vga.Height();

    cellW = scrW / COLS;
    cellH = scrH / ROWS;

    offsetX = (scrW - cellW * COLS) / 2;
    offsetY = (scrH - cellH * ROWS) / 2;

    initTables();

    // Очистим оба буфера
    vga.cls(0);
    vga.swap();
    vga.cls(0);

    vga.setTextSize(1);

    fpsTimer = millis();

    Serial.println("FAST 3D DOUGHNUT STARTED");
}

// ==========================================
// LOOP
// ==========================================

void loop() {

    if (!ready) return;

    uint32_t start = micros();

    // 1. Вычисляем 3D
    renderTorus();

    // 2. Быстро выводим в RGB565
    drawTorus();

    // 3. FPS на экране
    vga.putPrintf(
        scrW - 75,
        5,
        0xFFFF,
        "FPS:%lu",
        (unsigned long)drawFPS
    );

    uint32_t renderUs = micros() - start;

    // 4. Смена буфера
    uint32_t beforeSwap = micros();
    vga.swap();
    uint32_t swapUs = micros() - beforeSwap;

    // 5. Поворот
    A += 0.04f;
    B += 0.02f;

    if (A >= 6.2831853f) A -= 6.2831853f;
    if (B >= 6.2831853f) B -= 6.2831853f;

    // 6. Измеряем производительность
    fpsFrames++;
    renderTimeSum += renderUs;
    swapTimeSum += swapUs;

    uint32_t now = millis();
    uint32_t elapsed = now - fpsTimer;

    if (elapsed >= 1000) {

        drawFPS =
            fpsFrames * 1000UL / elapsed;

        float avgRender =
            renderTimeSum / (float)fpsFrames / 1000.0f;

        float avgSwap =
            swapTimeSum / (float)fpsFrames / 1000.0f;

        Serial.printf(
            "FPS:%lu | RENDER:%.2f ms | SWAP:%.2f ms\n",
            (unsigned long)drawFPS,
            avgRender,
            avgSwap
        );

        fpsFrames = 0;
        renderTimeSum = 0;
        swapTimeSum = 0;
        fpsTimer = now;
    }
}
