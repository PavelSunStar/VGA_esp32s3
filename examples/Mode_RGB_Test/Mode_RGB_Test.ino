#include <VGA_esp32s3.h>
#include <VGA_GFX.h>

VGA_esp32s3 vga;   
VGA_GFX gfx(vga); 

int scrX, scrY;

void draw() {
    gfx.cls(0xFFFF);
    int d = scrY / 3;
    
    for (int x = 0; x < scrX - 1; x++) {
        // растягиваем 0..255 на ширину экрана
        uint8_t colIndex = x * 256 / scrX;

        // верхняя полоса: красный от 0 до 255, зелёный и синий 0
        gfx.vLine(x + 1, 1, d, gfx.getCol(colIndex, 0, 0));

        // средняя полоса: зелёный от 0 до 255, красный и синий 0
        gfx.vLine(x + 1, d, d * 2, gfx.getCol(0, colIndex, 0));

        // нижняя полоса: синий от 0 до 255, красный и зелёный 0
        gfx.vLine(x + 1, d * 2, scrY - 1, gfx.getCol(0, 0, colIndex));
    }
}

void setup() { 
  //          by default
  //          r  r  r  r  r   g   g   g   g  g  g   b   b   b   b   b   h  v
  //          0  1  2  3  4   5   6   7   8  9  10  11  12  13  14  15  16 17
  vga.setPins(4, 5, 6, 7, 15, 16, 17, 18, 8, 9, 14, 10, 11, 12, 13, 21, 1, 2);

  vga.init(VGA_esp32s3::MODE640x480x16, 0, false, true);
  scrX = vga.ScrWidth() - 1;
  scrY = vga.ScrHeight() - 1; 

  draw();
}

void loop() {

}

