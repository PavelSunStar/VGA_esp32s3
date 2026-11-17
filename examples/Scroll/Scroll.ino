
#include <Arduino.h>
#include <VGA_esp32.h>
#include <string.h>

VGA_esp32 vga;

bool ready = false;

int scrW, scrH;

// Временная строка для прокрутки
uint16_t* tempLine = nullptr;

// ==========================================
// ПРОКРУТКА ОБЛАСТИ ПО X
// ==========================================

void winScrollX(
    int x1, int y1,
    int x2, int y2,
    int dx
) {
    if (!ready || !tempLine) return;

    x1 = constrain(x1, 0, scrW - 1);
    x2 = constrain(x2, 0, scrW - 1);
    y1 = constrain(y1, 0, scrH - 1);
    y2 = constrain(y2, 0, scrH - 1);

    int w = x2 - x1 + 1;
    if (w <= 1 || y2 < y1) return;

    dx %= w;
    if (dx < 0) dx += w;
    if (dx == 0) return;

    uint16_t* fb = (uint16_t*)vga.Buf();

    for (int y = y1; y <= y2; y++) {

        uint16_t* row = fb + y * scrW + x1;

        // Сохраняем правый край
        memcpy(
            tempLine,
            row + w - dx,
            dx * sizeof(uint16_t)
        );

        // Сдвигаем строку вправо
        memmove(
            row + dx,
            row,
            (w - dx) * sizeof(uint16_t)
        );

        // Переносим правый край налево
        memcpy(
            row,
            tempLine,
            dx * sizeof(uint16_t)
        );
    }
}

// ==========================================
// ПРОКРУТКА ОБЛАСТИ ПО Y
// ==========================================

void winScrollY(
    int x1, int y1,
    int x2, int y2,
    int dy
) {
    if (!ready || !tempLine) return;

    x1 = constrain(x1, 0, scrW - 1);
    x2 = constrain(x2, 0, scrW - 1);
    y1 = constrain(y1, 0, scrH - 1);
    y2 = constrain(y2, 0, scrH - 1);

    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;

    if (w < 1 || h <= 1) return;

    dy %= h;
    if (dy < 0) dy += h;
    if (dy == 0) return;

    uint16_t* fb = (uint16_t*)vga.Buf();
    size_t bytes = w * sizeof(uint16_t);

    // Прокручиваем на одну строку за шаг.
    // Содержимое переносится циклически.
    for (int step = 0; step < dy; step++) {

        uint16_t* last =
            fb + y2 * scrW + x1;

        memcpy(tempLine, last, bytes);

        for (int y = y2; y > y1; y--) {

            uint16_t* dst =
                fb + y * scrW + x1;

            uint16_t* src =
                fb + (y - 1) * scrW + x1;

            memmove(dst, src, bytes);
        }

        memcpy(
            fb + y1 * scrW + x1,
            tempLine,
            bytes
        );
    }
}

// ==========================================
// ПРОКРУТКА ОБЛАСТИ ПО XY
// ==========================================

void winScrollXY(
    int x1, int y1,
    int x2, int y2,
    int dx, int dy
) {
    winScrollX(x1, y1, x2, y2, dx);
    winScrollY(x1, y1, x2, y2, dy);
}

// ==========================================
// ПРОКРУТКА ВСЕГО ЭКРАНА
// ==========================================

void scrollX(int dx) {
    winScrollX(
        0, 0,
        scrW - 1, scrH - 1,
        dx
    );
}

void scrollY(int dy) {
    winScrollY(
        0, 0,
        scrW - 1, scrH - 1,
        dy
    );
}

void scrollXY(int dx, int dy) {
    winScrollXY(
        0, 0,
        scrW - 1, scrH - 1,
        dx, dy
    );
}

// ==========================================
// SETUP
// ==========================================

void setup() {

    Serial.begin(115200);

    // Один буфер RGB565
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

    scrW = vga.Width();
    scrH = vga.Height();

    tempLine = (uint16_t*)malloc(
        scrW * sizeof(uint16_t)
    );

    if (!tempLine) {
        Serial.println("TEMP BUFFER ERROR!");
        ready = false;
        return;
    }

    // Начальная тестовая картинка
    vga.testRGB();

    randomSeed(micros());

    Serial.println("VGA SCROLL STARTED!");
}

// ==========================================
// LOOP
// ==========================================

void loop() {

    if (!ready) return;

    int pause = 10;

    // Выбираем один из шести эффектов
    int effect = random(6);

    int move = random(10, 100);

    int dx = random(-2, 3);
    int dy = random(-2, 3);

    // Исключаем неподвижный кадр
    if (dx == 0 && dy == 0)
        dx = 1;

    int minSize = 25;

    int x1 = random(scrW - minSize);
    int x2 = random(x1 + minSize, scrW);

    int y1 = random(scrH - minSize);
    int y2 = random(y1 + minSize, scrH);

    Serial.printf(
        "Effect: %d | Move: %d | X:%d Y:%d\n",
        effect, move, dx, dy
    );

    switch (effect) {

        // Весь экран по X
        case 0:
            for (int i = 0; i < move; i++) {
                scrollX(dx);
                delay(pause);
            }
            break;

        // Весь экран по Y
        case 1:
            for (int i = 0; i < move; i++) {
                scrollY(dy);
                delay(pause);
            }
            break;

        // Весь экран по XY
        case 2:
            for (int i = 0; i < move; i++) {
                scrollXY(dx, dy);
                delay(pause);
            }
            break;

        // Окно по X
        case 3:
            for (int i = 0; i < move; i++) {
                winScrollX(
                    x1, y1, x2, y2, dx
                );
                delay(pause);
            }
            break;

        // Окно по Y
        case 4:
            for (int i = 0; i < move; i++) {
                winScrollY(
                    x1, y1, x2, y2, dy
                );
                delay(pause);
            }
            break;

        // Окно по XY
        case 5:
            for (int i = 0; i < move; i++) {
                winScrollXY(
                    x1, y1, x2, y2,
                    dx, dy
                );
                delay(pause);
            }
            break;
    }
}
