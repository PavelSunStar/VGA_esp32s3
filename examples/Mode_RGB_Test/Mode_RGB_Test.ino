#include <LCD_esp32s3.h>
#include <LCD_GFX.h>

LCD_esp32s3 vga;   
LCD_GFX gfx(vga); 

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
  vga.init(LCD_esp32s3::MODE640x480x16, 2, false, true);
  scrX = vga.getScrWidth() - 1; 
  scrY = vga.getScrHeight() - 1;

  //gfx.cls(0xffff);
  draw();
  //gfx.swap();

  int scrSize = vga.getWidth() * vga.getHeight();
  for (int y = 1; y < vga.getHeight(); y++){
    int size = vga.getWidth() * y;
    if (size % 64 == 0  && size > 5000 && size <= 60000){
       if (scrSize % size == 0 && size % vga.getScrWidth() == 0)
        Serial.printf("size = %d, blocks: %d\n", size, scrSize / size);
    }
  }
}

void loop() {

}

