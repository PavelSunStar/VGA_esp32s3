#include <Arduino.h>
#include "VGA_GFX.h"
#include <cstring>

VGA_GFX::VGA_GFX(VGA_esp32s3& vga) : _vga(vga){

}

VGA_GFX::~VGA_GFX(){

}

uint16_t VGA_GFX::getCol(uint8_t r, uint8_t g, uint8_t b) {
    if (_vga.getBpp() == 16) {
        // формируем 16-битный цвет RGB565
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);      
    } 
    else if (_vga.getBpp() == 8) {
        // формируем 8-битный цвет RGB332
        return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6); 
    }
    
    return 0;
}

void VGA_GFX::cls(uint16_t col){
    if (_vga.getBpp() == 16) {
        uint16_t* scr = _vga._buf16 + _vga._backBuff;
        uint16_t* savePos = scr;

        int sizeX;
        int skip = sizeX = _vga.getScrWidth();
        while (sizeX-- > 0) *scr++ = col;

        int skip2X = skip << 1;
        int sizeY = _vga.getScrYY();
        while (sizeY-- > 0){
            memcpy(scr, savePos, skip2X);
            scr += skip;
        }
    } else {
        uint8_t* scr = _vga._buf8 + _vga._backBuff;
        memset(scr, (uint8_t)col, _vga.getScrSize());
    }
}

void VGA_GFX::putPixel(int x, int y, uint16_t col) {
    if (x < _vga.get_vX1() || y < _vga.get_vY1() || x > _vga.get_vX2() ||  y > _vga.get_vY2()) return;

    if (_vga.getBpp() == 16){
        uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y) + x;
        *scr = col;
    } else {
        uint8_t* scr = _vga._buf8 + _vga._backBuff + *(_vga._fastY + y) + x;
        *scr = (uint8_t)col;
    }
}

void VGA_GFX::hLine(int x1, int y, int x2, uint16_t col){
    if (y < _vga.get_vY1() || y > _vga.get_vY2()) return;

    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(_vga.get_vX1(), x1);
    x2 = std::min(_vga.get_vX2(), x2);
    int sizeX = x2 - x1 + 1;
    
    if (_vga.getBpp() == 16){
        uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y) + x1;
        while (sizeX-- > 0) *scr++ = col; 
    } else {
        uint8_t* scr = _vga._buf8 + _vga._backBuff + *(_vga._fastY + y) + x1;
        memset(scr, (uint8_t)col, sizeX);
    }    
}

void VGA_GFX::vLine(int x, int y1, int y2, uint16_t col){
    if (x < _vga.get_vX1() || x > _vga.get_vX2()) return;

    if (y1 > y2) std::swap(y1, y2);
    y1 = std::max(_vga.get_vY1(), y1);
    y2 = std::min(_vga.get_vY2(), y2);
    int sizeY = y2 - y1 + 1;
    int skip = _vga.getScrWidth();
    
    if (_vga.getBpp() == 16){
        uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y1) + x;

        while (sizeY-- > 0){ 
            *scr = col;
            scr += skip;
        }     
    } else {
        uint8_t* scr = _vga._buf8 + _vga._backBuff + *(_vga._fastY + y1) + x;

        uint8_t color = (uint8_t)col;
        while (sizeY-- > 0){ 
            *scr = color;
            scr += skip;
        }     
    }  
}

void VGA_GFX::rect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (x1 > _vga.get_vX2() || y1 > _vga.get_vY2() || x2 < _vga.get_vX1() || y2 < _vga.get_vY1()){
        return;
    } else if (x1 >= _vga.get_vX1() && y1 >= _vga.get_vY1() && x2 <= _vga.get_vX2() && y2 <= _vga.get_vY2()){
        int sizeX = x2 - x1 + 1; 
        int sizeY = y2 - y1 - 1;
        int width = _vga.getScrWidth();
        int skip1 = x2 - x1; 
        int skip2 = width - x2 + x1;
        
        if (_vga.getBpp() == 16){
            uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y1) + x1;
            int saveSizeX = sizeX;
            
            while (sizeX-- > 0) *scr++ = col;
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            while (saveSizeX-- > 0) *scr++ = col;
        } else {
            uint8_t* scr = _vga._buf8 + _vga._backBuff + *(_vga._fastY + y1) + x1;
            uint8_t color = (uint8_t) col;

            memset(scr, color, sizeX);
            scr += width;

            while (sizeY-- > 0){
                *scr = color; scr += skip1;
                *scr = color; scr += skip2;
            }
            memset(scr, color, sizeX);
        }
    } else {
        hLine(x1, y1, x2, col); 
        hLine(x1, y2, x2, col); 
        vLine(x1, y1, y2, col);  
        vLine(x2, y1, y2, col);  
    }    
}

void VGA_GFX::fillRect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 > _vga.get_vX2() || y1 > _vga.get_vY2() || x2 < _vga.get_vX1() || y2 < _vga.get_vY1()) return; 

    x1 = std::max(_vga.get_vX1(), x1);
    x2 = std::min(_vga.get_vX2(), x2);
    y1 = std::max(_vga.get_vY1(), y1);
    y2 = std::min(_vga.get_vY2(), y2);
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;     
    int width = _vga.getScrWidth();

    if (_vga.getBpp() == 16){
        uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y1) + x1;
        uint16_t* savePos = scr;
        int skip1 = width << 1;
        int skip2 = width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX;

        while (sizeX-- > 0) *scr++ = col;
        scr += skip2;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, savePos, copyBytes);
            scr += width;
        }
    } else {
        uint8_t* scr = _vga._buf8 + _vga._backBuff + *(_vga._fastY + y1) + x1;
        uint8_t color = (uint8_t)col;
        
        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += width;
        }    
    }     
}

void VGA_GFX::line(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 == x2) {
        vLine(x1, y1, y2, col);
    } else if (y1 == y2){
        hLine(x1, y1, x2, col);
    } else {
        // Вычисляем разницу по осям X и Y
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);   

       // Определяем направление движения по осям
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
    
        // Инициализируем ошибку
        int err = dx - dy; 

        int xx1 = _vga.get_vX1();
        int yy1 = _vga.get_vY1();
        int xx2 = _vga.get_vX2();
        int yy2 = _vga.get_vY2();

        if (_vga.getBpp() == 16){
            uint16_t* scr = _vga._buf16 + _vga._backBuff;
            uint16_t* tmp = scr;

            while (true){
                if (x1 >= _vga.get_vX1() && x1 <= _vga.get_vX2() &&
                    y1 >= _vga.get_vY1() && y1 <= _vga.get_vY2()){
                
                    scr = tmp + *(_vga._fastY + y1) + x1;
                    *scr = col;
                }

                if (x1 == x2 && y1 == y2) break;

                int e2 = 2 * err;
                if (e2 > -dy){ err -= dy; x1 += sx; }
                if (e2 < dx){ err += dx; y1 += sy; }
            }
        } else {
            uint8_t* scr = _vga._buf8 + _vga._backBuff;
            uint8_t* tmp = scr;            
            uint8_t color = (uint8_t)col;

            while (true){
                if (x1 >= _vga.get_vX1() && x1 <= _vga.get_vX2() &&
                    y1 >= _vga.get_vY1() && y1 <= _vga.get_vY2()){
                    uint8_t* scr = tmp + *(_vga._fastY + y1) + x1;
                    *scr = color;
                }

                if (x1 == x2 && y1 == y2) break;

                int e2 = 2 * err;
                if (e2 > -dy){ err -= dy; x1 += sx; }
                if (e2 < dx){ err += dx; y1 += sy; }
            }
        }   
    }    
}

void VGA_GFX::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

/*


void VGA_GFX::initGFX(){
    // Выбираем реализацию один раз
    if (_vga.getBpp() == 16)
        putPixelFunc = &VGA_GFX::putPixel16_static;
    else
        putPixelFunc = &VGA_GFX::putPixel8_static;    
}

// 16-битная реализация
void VGA_GFX::putPixel16_static(VGA_GFX* self, int x, int y, uint16_t col) {
    if (x < self->_vga.get_vX1() || y < self->_vga.get_vY1() ||
        x > self->_vga.get_vX2() || y > self->_vga.get_vY2()) return;

    int offset = self->_vga._backBuff + *(self->_vga._fastY + y) + x;
    self->_vga._buf16[offset] = col;
}

// 8-битная реализация
void VGA_GFX::putPixel8_static(VGA_GFX* self, int x, int y, uint16_t col) {
    if (x < self->_vga.get_vX1() || y < self->_vga.get_vY1() ||
        x > self->_vga.get_vX2() || y > self->_vga.get_vY2()) return;

    int offset = self->_vga._backBuff + *(self->_vga._fastY + y) + x;
    self->_vga._buf8[offset] = (uint8_t)col;
}
*/