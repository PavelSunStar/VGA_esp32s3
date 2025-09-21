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
  vga.init(VGA_esp32s3::MODE640x480x16, 1, false, true);
  scrX = vga.ScrWidth() - 1;
  scrY = vga.ScrHeight() - 1; 

  draw();
}

void loop() {
  int pause = 10;
  int set = random(6);
  int move = random(10, scrX);
  int x = random(5) - 2;
  int y = random(5) - 2;

  int minSize = 25;
  int x1 = random(scrX - minSize);
  int x2 = x1 + random(minSize, scrX - x1);
  int y1 = random(scrY - minSize);
  int y2 = y1 + random(minSize, scrY - y1);
  
  switch (set) {
      case 0:{
        for (int i = 0; i < move; i++)
          vga.scrollX(x);
        break;
      }

      case 1:{
        for (int i = 0; i < move; i++)
          vga.scrollY(x);
        break;
      }

      case 2:{
        for (int i = 0; i < move; i++)
          vga.scrollXY(x, y);
        break;
      }

      case 3:{
        for (int i = 0; i < move; i++){
          vga.winScrollX(x1, y1, x2, y2, x);
          delay(pause);
        }  
        break;
      }
      
      case 4:{
        for (int i = 0; i < move; i++){
          vga.winScrollY(x1, y1, x2, y2, y);
          delay(pause);
        }  
        break;
      }
      
      case 5:{
        for (int i = 0; i < move; i++){
          vga.winScrollXY(x1, y1, x2, y2, x, y);
          delay(pause);
        }
        break;
      }      
    }
}

