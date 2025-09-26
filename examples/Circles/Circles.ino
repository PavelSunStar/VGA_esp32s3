//https://github.com/bitluni/ESP32Lib/blob/master/examples/VGADemo14Bit/VGADemo14Bit.ino
#include <VGA_esp32s3.h>
#include <VGA_GFX.h>
#include <VGA_Math.h>

VGA_esp32s3 vga;
VGA_GFX gfx(vga);
VGA_Math math;

// параметры шариков
int scrX3, scrY3;
uint16_t colors[] = {0b1111100000000000, 0b0000011111100000, 0b0000000000011111};
//uint8_t colors[] = {0b11100000, 0b00011100, 0b00000011};
float factors[][2] = {{1, 1.1f}, {0.9f, 1.02f}, {1.1f, 0.8f}};

void setup() {
    vga.init(VGA_esp32s3::MODE640x480x16, 1, false, true);
    scrX3 = vga.ScrWidth() / 3;
    scrY3 = vga.ScrHeight() / 3;
    math.startFPS();
}

void loop() {
  math.calcFPS();

  float p = millis() * 0.002f;

  // рисуем шарики
  for(int i = 0; i < 3; i++) {
    int x = vga.ScrCX() + sinf(p * factors[i][0]) * scrX3;
    int y = vga.ScrCY() + cosf(p * factors[i][1]) * scrY3;

    gfx.circle(x, y, 25, colors[i]);
  }

  gfx.blur();
  Serial.println(math.getFPS());
}
