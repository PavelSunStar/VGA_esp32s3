//https://github.com/MinhasKamal/CreepyCodeCollection/blob/master/animated_3d_doughnut.c
#include "VGA_esp32s3.h"
#include "VGA_GFX.h"

VGA_esp32s3 vga;
VGA_GFX gfx(vga);

// Размер "символьной сетки" (как консоль 80×40)
const int cols = 80;
const int rows = 40;

float zbuf[cols * rows];
uint8_t cbuf[cols * rows];

// Таблицы синусов/косинусов
#define STEPS_I 314   // 6.28 / 0.02 ≈ 314 шагов
#define STEPS_J 90    // 6.28 / 0.07 ≈ 90 шагов
float sin_i[STEPS_I], cos_i[STEPS_I];
float sin_j[STEPS_J], cos_j[STEPS_J];

int scrW, scrH;
int cellW, cellH;

float A = 0, B = 0;

void initTables() {
  for (int k = 0; k < STEPS_I; k++) {
    float angle = k * 0.02f;
    sin_i[k] = sin(angle);
    cos_i[k] = cos(angle);
  }
  for (int k = 0; k < STEPS_J; k++) {
    float angle = k * 0.07f;
    sin_j[k] = sin(angle);
    cos_j[k] = cos(angle);
  }
}

void setup() {
  vga.init(VGA_esp32s3::MODE512x384x16, 2, true, true);

  scrW = vga.ScrWidth();   // 128
  scrH = vga.ScrHeight();  // 96

  cellW = scrW / cols; // ширина ячейки ≈ 1–2 пикселя
  cellH = scrH / rows; // высота ячейки ≈ 2 пикселя

  initTables();
}

void loop() {
  // очистка буферов
    memset(zbuf, 0, sizeof(zbuf));
    memset(cbuf, 0, sizeof(cbuf));

  // рендер тора
  for (int jj = 0; jj < STEPS_J; jj++) {
    float d = cos_j[jj];
    float f = sin_j[jj];

    for (int ii = 0; ii < STEPS_I; ii++) {
      float c = sin_i[ii];
      float l = cos_i[ii];

      float e = sin(A), g = cos(A);
      float m = cos(B), n = sin(B);

      float h = d + 2;
      float D = 1.0f / (c * h * e + f * g + 5);
      float t = c * h * g - f * e;

      int x = (int)(cols / 2 + (cols / 2) * D * (l * h * m - t * n));
      int y = (int)(rows / 2 + (rows / 2) * D * (l * h * n + t * m));

      if (x < 0 || x >= cols || y < 0 || y >= rows) continue;

      int o = x + cols * y;

      int N = (int)(8 * ((f * e - c * d * g) * m
                        - c * d * e - f * g - l * d * n));

      if (N < 0) N = 0;

      if (D > zbuf[o]) {
        zbuf[o] = D;
        cbuf[o] = N;
      }
    }
  }

  // выводим на VGA
  gfx.cls(0);
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      int val = cbuf[x + y * cols];
      if (val > 0) {
        // заливаем ячейку цветом val
        for (int yy = 0; yy < cellH; yy++) {
          for (int xx = 0; xx < cellW; xx++) {
            gfx.putPixel(x * cellW + xx + 24, y * cellH + yy + 10, val);
          }
        }
      }
    }
  }

  vga.swap();

  // вращение
  A += 0.04;
  B += 0.02;
}
