#include <Arduino.h>
#include "VGA_Sprite.h"

VGA_Sprite::VGA_Sprite(VGA_esp32s3& vga) : _vga(vga){

}   

VGA_Sprite::~VGA_Sprite(){ 

}

void VGA_Sprite::destroy() {
    if (_img16) {
        heap_caps_free(_img16);
        _img16 = nullptr;
    }

    if (_img8) {
        heap_caps_free(_img8);
        _img8 = nullptr;
    }

    if (_aniOffset){
        free(_aniOffset); 
        _aniOffset = nullptr;
    }

    _created = false;
}

bool VGA_Sprite::allocateMemory(int xx, int yy) {
    destroy();
    if (_size == 0) return false;  // защита

    if (_bpp == 16) {
        _img16 = (uint16_t*)heap_caps_malloc(_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (_img16 == nullptr) return false;
        memset(_img16, 0, _size);
    } else {
        _img8 = (uint8_t*)heap_caps_malloc(_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (_img8 == nullptr) return false;
        memset(_img8, 0, _size);
    }

    return true;
}

bool VGA_Sprite::create(int xx, int yy, int num) {
    if (xx <= 0 || yy <= 0 || num == 0) return false;

    _scrWidth  = _vga.getScrWidth();
    _scrHeight = _vga.getScrHeight();
    _bpp       = _vga.getBpp();

    _width  = xx; _xx = _width - 1;
    _height = yy; _yy = _height - 1;
    _aniHeight = _height * num;
    _aniMax = num - 1;

    size_t frameSize = static_cast<size_t>(_width) * _height * 
                       (_bpp == 16 ? sizeof(uint16_t) : sizeof(uint8_t));
    _size = frameSize * num;
    Serial.printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",  _scrWidth, _scrHeight, _bpp, _width, _xx, _height, _yy, _aniHeight, _aniMax, frameSize, _size);
    _created = allocateMemory(xx, _aniHeight);  // выделяем всю ленту кадров

    if (_created) {
        _aniOffset = (size_t*) malloc(num * sizeof(size_t));
        if (_aniOffset) {
            for (int i = 0; i < num; i++) {
                _aniOffset[i] = frameSize * i; // смещения в байтах
            }
        } else {
            _created = false;
            return false;
        }
    }

    Serial.printf("Image created: %dx%dx%d, frames: %d, aniHeight:%d\n",
                  _width, _height, _bpp, num, _aniHeight);

    return _created;    
}

void VGA_Sprite::putImage(int x, int y, int num, bool mirror_y) {
    if (!_created) return;
    num = std::clamp(num, 0, _aniMax);

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    if (x > _vga.get_vX2() || y > _vga.get_vY2() || x2 < _vga.get_vX1() || y2 < _vga.get_vY1())
        return; // полностью за пределами экрана

    // Координаты начала отрисовки
    int dx = std::max(x, _vga.get_vX1());
    int dy = std::max(y, _vga.get_vY1());

    // Смещение внутри спрайта
    int sx = dx - x;
    int sy = dy - y;

    // Размер отрисовываемой части
    int sizex = std::min(x2, _vga.get_vX2()) - dx + 1;
    int sizey = std::min(y2, _vga.get_vY2()) - dy + 1;
    int imgAdd = mirror_y ? -_width : _width;

    if (_bpp == 16) {
        uint16_t* dest = _vga._buf16 + _vga._backBuff + *(_vga._fastY + dy) + dx;
        uint16_t* sour = _img16 + _width * (mirror_y ? _xx - sy : sy) + sx + _aniOffset[num];

        while (sizey-- > 0) {
            memcpy(dest, sour, sizex * sizeof(uint16_t));
            dest += _scrWidth;
            sour += imgAdd;
        }
    } else {
        uint8_t* dest = _vga._buf8 + _vga._backBuff + *(_vga._fastY + dy) + dx;
        uint8_t* sour = _img8 + _width * (mirror_y ? _xx - sy : sy) + sx + _aniOffset[num];

        while (sizey-- > 0) {
            memcpy(dest, sour, sizex);
            dest += _scrWidth;
            sour += imgAdd;
        }
    }
}

void VGA_Sprite::putSprite(int x, int y, uint16_t maskColor, int num, bool mirror_x, bool mirror_y) {
    if (!_created) return;
    num = std::clamp(num, 0, _aniMax);

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    if (x > _vga.get_vX2() || y > _vga.get_vY2() || x2 < _vga.get_vX1() || y2 < _vga.get_vY1())
        return; // полностью за пределами экрана

    // Координаты начала отрисовки
    int dx = std::max(x, _vga.get_vX1());
    int dy = std::max(y, _vga.get_vY1());

    // Смещение внутри спрайта
    int sx = dx - x;
    int sy = dy - y;

    // Размер отрисовываемой части
    int sizeX = std::min(x2, _vga.get_vX2()) - dx + 1;
    int sizeY = std::min(y2, _vga.get_vY2()) - dy + 1;
    int imgAdd = mirror_y ? -_width : _width;

    if (_bpp == 16) {
        uint16_t* dest = _vga._buf16 + _vga._backBuff + *(_vga._fastY + dy) + dx;
        uint16_t* sour = _img16 + _width * (mirror_y ? _xx - sy : sy) + sx + _aniOffset[num];

        while (sizeY-- > 0) {
            memcpy(dest, sour, sizeX * sizeof(uint16_t));
            dest += _scrWidth;
            sour += imgAdd;
        }
    } else {
        uint8_t* dest = _vga._buf8 + _vga._backBuff + *(_vga._fastY + dy) + dx;
        uint8_t* sour = _img8 + _width * (mirror_y ? _xx - sy : sy) + sx + _aniOffset[num];

        uint8_t color = (uint8_t)maskColor;
        int skip1 = _scrWidth - sizeX;
        
        if (!mirror_x){
            int skip2 = imgAdd - sizeX;
            
            while (sizeY-- > 0) {
                int tmp = sizeX;

                while (tmp-- > 0) {
                    if (*sour != color) *dest = *sour;
                    dest++;
                    sour++;
                }

                dest += skip1;
                sour += skip2;
            }
        } else {
            sour--;
            while (sizeY-- > 0) {
                int tmp = sizeX;
                int skip2 = imgAdd + sizeX;
                sour += tmp;

                while (tmp-- > 0) {
                    if (*sour != color) *dest = *sour;
                    dest++;
                    sour--;
                }

                dest += skip1;
                sour += imgAdd; 
            }                 
        }
    }
}

//Draw in sprite---------------------------------------------------------------------------
void VGA_Sprite::cls(uint16_t col) {
    if (_bpp == 16) {
        // заполнение первой строки
        std::fill_n(_img16, _width, col);

        // копирование на остальные строки
        size_t rowSize = _width * sizeof(uint16_t);
        for (int y = 1; y < _yy; y++) {
            memcpy(_img16 + y * _width, _img16, rowSize);
        }
    } else {
        // 8 bpp - палитра
        memset(_img8, (uint8_t)col, _size);
    }
}

void VGA_Sprite::putPixel(int x, int y, uint16_t col) {
    if (!_created || x < 0 || y < 0 || x > _xx || y > _yy) return;

    if (_bpp == 16) {
        uint16_t* img = _img16 + y * _width + x;
        *img = col;
    } else {
        uint8_t* img = _img8 + y * _width + x;
        *img = (uint8_t)(col);
    }
}

void VGA_Sprite::hLine(int x1, int y, int x2, uint16_t col){
    if (y < 0 || y > _yy) return;

    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(0, x1);
    x2 = std::min(_xx, x2);
    int sizeX = x2 - x1 + 1;
    
    if (_bpp == 16){
        uint16_t* img = _img16 + _width * y + x1;
        while (sizeX-- > 0) *img++ = col; 
    } else {
        uint8_t* img = _img8 + _width * y + x1;
        memset(img, (uint8_t)col, sizeX);
    }    
}

void VGA_Sprite::vLine(int x, int y1, int y2, uint16_t col){
    if (x < 0 || x > _xx) return;

    if (y1 > y2) std::swap(y1, y2);
    y1 = std::max(_vga.get_vY1(), y1);
    y2 = std::min(_vga.get_vY2(), y2);
    int sizeY = y2 - y1 + 1;
    int skip = _width;
    
    if (_bpp == 16){
        uint16_t* img = _img16 + _width * y1 + x;

        while (sizeY-- > 0){ 
            *img = col;
            img += skip;
        }     
    } else {
        uint8_t* img = _img8 + _width * y1 + x;

        uint8_t color = (uint8_t)col;
        while (sizeY-- > 0){ 
            *img = color;
            img += skip;
        }     
    }  
}

void VGA_Sprite::rect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (x1 > _xx || y1 > _yy || x2 < 0 || y2 < 0){
        return;
    } else if (x1 >= 0 && y1 >= 0 && x2 <= _xx && y2 <= _yy){
        int sizeX = x2 - x1 + 1; 
        int sizeY = y2 - y1 - 1;
        int skip1 = x2 - x1; 
        int skip2 = _width - x2 + x1;
        
        if (_bpp == 16){
            uint16_t* img = _img16 + _width * y1 + x1;
            int saveSizeX = sizeX;
            
            while (sizeX-- > 0) *img++ = col;
            img += skip2 - 1;

            while (sizeY-- > 0){
                *img = col; img += skip1;
                *img = col; img += skip2;
            }
            while (saveSizeX-- > 0) *img++ = col;
        } else {
            uint8_t* img = _img8 + _width * y1 + x1;
            uint8_t color = (uint8_t) col;

            memset(img, color, sizeX);
            img += _width;

            while (sizeY-- > 0){
                *img = color; img += skip1;
                *img = color; img += skip2;
            }
            memset(img, color, sizeX);
        }
    } else {
        hLine(x1, y1, x2, col); 
        hLine(x1, y2, x2, col); 
        vLine(x1, y1, y2, col);  
        vLine(x2, y1, y2, col);  
    }    
}

void VGA_Sprite::fillRect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 > _xx || y1 > _yy || x2 < 0 || y2 < 0) return; 

    x1 = std::max(0, x1);
    x2 = std::min(_xx, x2);
    y1 = std::max(0, y1);
    y2 = std::min(_yy, y2);
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;     

    if (_bpp == 16){
        uint16_t* img = _img16 + _width * y1 + x1;
        uint16_t* savePos = img;

        int skip1 = _width << 1;
        int skip2 = _width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX;

        while (sizeX-- > 0) *img++ = col;
        img += skip2;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(img, savePos, copyBytes);
            img += _width;
        }
    } else {
        uint8_t* img = _img8 + _width * y1 + x1;
        uint8_t color = (uint8_t)col;
        
        while (sizeY-- > 0){
            memset(img, color, sizeX);
            img += _width;
        }    
    }     
}

void VGA_Sprite::line(int x1, int y1, int x2, int y2, uint16_t col){
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

        if (_bpp == 16){
            uint16_t* img = _img16;

            while (true){
                if (x1 >= 0 && x1 <= _xx && y1 >= 0 && y1 <= _yy){                
                    img = _img16 + _width * y1 + x1;
                    *img = col;
                }

                if (x1 == x2 && y1 == y2) break;

                int e2 = 2 * err;
                if (e2 > -dy){ err -= dy; x1 += sx; }
                if (e2 < dx){ err += dx; y1 += sy; }
            }
        } else {
            uint8_t* img = _img8;
            uint8_t color = (uint8_t)col;

            while (true){
                if (x1 >= 0 && x1 <= _xx && y1 >= 0 && y1 <= _yy){
                    img = _img8 + _width * y1 + x1;
                    *img = color;
                }

                if (x1 == x2 && y1 == y2) break;

                int e2 = 2 * err;
                if (e2 > -dy){ err -= dy; x1 += sx; }
                if (e2 < dx){ err += dx; y1 += sy; }
            }
        }   
    }    
}

void VGA_Sprite::lineAngle(int x, int y, int len, int angle, uint16_t col){
    int x1 = _math.xLUT(x, y, len, angle);
    int y1 = _math.yLUT(x, y, len, angle);
    line(x, y, x1, y1, col);
}

void VGA_Sprite::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

void VGA_Sprite::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    // Сортируем вершины по y (y1 <= y2 <= y3)
    if (y1 > y2){ std::swap(y1, y2); std::swap(x1, x2); }
    if (y1 > y3){ std::swap(y1, y3); std::swap(x1, x3); }
    if (y2 > y3){ std::swap(y2, y3); std::swap(x2, x3); }

    auto drawScanline = [&](int y, int xStart, int xEnd){
        hLine(xStart, y, xEnd, col);
    };

    auto edgeInterp = [](int y1, int x1, int y2, int x2, int y) -> int {
        if (y2 == y1) return x1; // избежать деления на 0
        return x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    };

    // Верхняя часть треугольника
    for (int y = y1; y <= y2; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y1, x1, y2, x2, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }

    // Нижняя часть треугольника
    for (int y = y2 + 1; y <= y3; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y2, x2, y3, x3, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }
}

void VGA_Sprite::circle(int xc, int yc, int r, uint16_t col){
    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    auto drawCircleSegments = [&](int x, int y){
        // Рисуем горизонтальные линии длиной 1 для верх/низ
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        if (x != y){
            putPixel(xc + y, yc + x, col);
            putPixel(xc - y, yc + x, col);
            putPixel(xc + y, yc - x, col);
            putPixel(xc - y, yc - x, col);
        }
    };

    while (y >= x){
        drawCircleSegments(x, y);
        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }
}

void VGA_Sprite::fillCircle(int xc, int yc, int r, uint16_t col){
    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    // верхняя и нижняя линии по центру
    hLine(xc - r, yc, xc + r, col);

    while (y >= x){
        // Рисуем линии для 4-х сегментов (верх/низ) одной итерацией
        if (x > 0){
            hLine(xc - y, yc + x, xc + y, col);
            hLine(xc - y, yc - x, xc + y, col);
        }
        if (y > x){
            hLine(xc - x, yc + y, xc + x, col);
            hLine(xc - x, yc - y, xc + x, col);
        }

        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }
}