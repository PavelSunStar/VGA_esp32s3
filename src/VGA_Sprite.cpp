#include <Arduino.h>
#include "VGA_Sprite.h"

VGA_Sprite::VGA_Sprite(VGA_esp32s3& vga) : _vga(vga){

}

VGA_Sprite::~VGA_Sprite(){

}

bool VGA_Sprite::allocateMemory(int size){
    uint32_t caps = MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    if (_bpp == 16){
        _img16 = (uint16_t*)heap_caps_malloc(_fullSize , caps);
        assert(_img16);
        memset(_img16, 0, _fullSize);
    } else {
        _img8 = (uint8_t*)heap_caps_malloc(_fullSize , caps);
        assert(_img8);
        memset(_img8, 0, _fullSize);
    }

    _imgOffset = (int*) malloc((_maxFrame + 1) * sizeof(int));
    for (int i = 0; i <= _maxFrame; i++)
        _imgOffset[i] = (_fullFrameSize * i);

    return true;
}

bool VGA_Sprite::create(int xx, int yy, int num){
    if (num < 1) return false;

    _scrWidth   = _vga.ScrWidth();
    _scrHeight  = _vga.ScrHeight();
    _bpp        = _vga.BPP();
    _bppShift   = _vga.BPPShift();

    _width      = xx;
    _height     = yy;
    _fullHeight = _height * num;
    _xx         = _width - 1;
    _yy         = _height - 1;
    _cx         = _width >> 1;
    _cy         = _height >> 1;

    _frameSize      = _width * _height;
    _fullFrameSize  = _frameSize << _bppShift;
    _fullSize       = _fullFrameSize * num;
    _maxFrame       = num;
    _copyBytes      = _width << _bppShift;
    _scrSkip        = _scrWidth << _bppShift;
    _created        = allocateMemory(_fullSize);

    if (_created) 
        Serial.printf("Create image: Width - %d, Height - %d, Frames - %d, Size - %d", 
                        _width, _height, _maxFrame, _fullSize);

    return _created;
}

void VGA_Sprite::destroy(){
    if (_img16) {
        heap_caps_free(_img16);
        _img16 = nullptr;
    }

    if (_img8) {
        heap_caps_free(_img8);
        _img8 = nullptr;
    }

    free(_imgOffset); _imgOffset = nullptr;    
} 
//===Draw Image===
void VGA_Sprite::putImage(int x, int y, int num, bool mirror_y) {
    if (!_created || num < 0 || num >= _maxFrame || x > _vga.vX2() || y > _vga.vY2()) return;
    if (x + _width < _vga.vX1() || y + _height < _vga.vY1()) return;

    // Указатель на экранный буфер
    uint8_t* scr = (_bpp == 16 ? PTR_OFFSET_T(_vga._buf16, _vga._backBuff, uint8_t)
                               : PTR_OFFSET_T(_vga._buf8,  _vga._backBuff, uint8_t));
    

    // Указатель на изображение (начало кадра)
    uint8_t* img = (_bpp == 16 ? PTR_OFFSET_T(_img16, *(_imgOffset + num), uint8_t)
                               : PTR_OFFSET_T(_img8,  *(_imgOffset + num), uint8_t));

    int x1 = max(_vga.vX1(), x);
    int y1 = max(_vga.vY1(), y);
    int x2 = min(_vga.vX2(), x + _width);
    int y2 = min(_vga.vY2(), y + _height);

    int sx = (x < _vga.vX1()) ? _vga.vX1() - x : 0;
    int sy = (y < _vga.vY1()) ? _vga.vY1() - y : 0;

    int sizeX = x2 - x1;    
    int sizeY = y2 - y1;   
    if (sizeX <= 0 || sizeY <= 0) return;

    int copyBytes = sizeX << _bppShift;
    int imgSkip = _width << _bppShift;
    scr += (*(_vga._fastY + y1) + x1) << _bppShift;

    if (mirror_y) {
        img += ((sy + sizeY - 1) * _width + sx) << _bppShift;

        while (sizeY-- > 0) {
            memcpy(scr, img, copyBytes);
            scr += _scrSkip;
            img -= imgSkip;   
        }
    } else {
        img += (sy * _width + sx) << _bppShift;

        while (sizeY-- > 0){
            memcpy(scr, img, copyBytes);
            scr += _scrSkip;
            img += imgSkip;
        }
    }    
}

//===Draw in sprite===
void VGA_Sprite::cls(uint16_t col, int num){
    if (!_created || num < 0 || num >= _maxFrame) return;

    if (_bpp == 16){
        uint16_t* img = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t); 
        uint16_t* savePos = img;

        int width = _width;
        while (width-- > 0) *img++ = col;
        
        int height = _yy;
        while (height-- > 0){
            memcpy(img, savePos, _copyBytes);
            img += _scrSkip;
        }
    } else {
        uint8_t* img = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
        memset(img, (uint8_t)col, _frameSize);
    }
}

void VGA_Sprite::putPixel(int x, int y, uint16_t col, int num) {
    if (!_created || num < 0 || num >= _maxFrame || x < 0 || y < 0 || x >= _width || y >= _height) return;

    int offset = y * _width + x;

    if (_bpp == 16) {
        uint16_t* img = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
        img[offset] = col;
    } else {
        uint8_t* img = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
        img[offset] = (uint8_t)col;
    }
}

void VGA_Sprite::hLine(int x1, int y, int x2, uint16_t col, int num) {
    if (!_created || num < 0 || num >= _maxFrame || y < 0 || y > _yy) return;

    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(0, x1);
    x2 = std::min(_xx, x2);

    int sizeX = x2 - x1 + 1;
    if (sizeX <= 0) return;
    int offset = y * _width + x1;

    if (_bpp == 16) {
        uint16_t* img = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
        img += offset;
        while (sizeX-- > 0) *img++ = col;
    } else {
        uint8_t* img = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
        img += offset;
        memset(img, (uint8_t)col, sizeX);
    }
}

void VGA_Sprite::vLine(int x, int y1, int y2, uint16_t col, int num){
    if (!_created || num < 0 || num >= _maxFrame || x < 0 || x > _xx) return;

    if (y1 > y2) std::swap(y1, y2);
    y1 = std::max(0, y1);
    y2 = std::min(_yy, y2);

    int sizeY = y2 - y1 + 1;
    if (sizeY <= 0) return;
    int offset = y1 * _width + x;

    if (_bpp == 16) {
        uint16_t* img = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
        img += offset;
        while (sizeY-- > 0){
            *img = col;
            img += _width;
        }    
    } else {
        uint8_t* img = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
        img += offset;
        while (sizeY-- > 0){
            *img = col;
            img += _width;
        }    
    }
}

void VGA_Sprite::rect(int x1, int y1, int x2, int y2, uint16_t col, int num){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (x1 > _width || y1 > _height || x2 < 0 || y2 < 0){
        return;
    } else if (x1 >= 0 && y1 >= 0 && x2 <= _width && y2 <= _height){
        int sizeX = x2 - x1 + 1; 
        int sizeY = y2 - y1 - 1;
        if (sizeX <= 0 || sizeY <= 0) return;

        int skip1 = x2 - x1; 
        int skip2 = _width - x2 + x1;
        int offset = y1 * _width + x1;

        if (_bpp == 16){
            uint16_t* scr = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
            scr += offset;
            int saveSizeX = sizeX;
            
            while (sizeX-- > 0) *scr++ = col;
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            while (saveSizeX-- > 0) *scr++ = col;
        } else {
            uint8_t* scr = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
            scr += offset;
            uint8_t color = (uint8_t) col;

            memset(scr, color, sizeX);
            scr += _width;

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

void VGA_Sprite::fillRect(int x1, int y1, int x2, int y2, uint16_t col, int num){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 > _width || y1 > _height || x2 < 0 || y2 < 0) return; 

    x1 = std::max(0, x1);
    x2 = std::min(_width, x2);
    y1 = std::max(0, y1);
    y2 = std::min(_height, y2);
    
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;     
    int offset = y1 * _width + x1;

    if (_bpp == 16){
        uint16_t* scr = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
        scr += offset;
        uint16_t* savePos = scr;

        int skip = _width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX; 

        while (sizeX-- > 0) *scr++ = col;
        scr += skip;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, savePos, copyBytes);
            scr += _width;
        }
    } else {
        uint8_t* scr = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
        scr += offset;
        uint8_t color = (uint8_t)col;
        
        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += _width;
        }    
    }     
}

void VGA_Sprite::line(int x1, int y1, int x2, int y2, uint16_t col, int num){
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

        int xx1 = 0;
        int yy1 = 0;
        int xx2 = _width;
        int yy2 = _height;

        if (_vga.BPP() == 16){
            uint16_t* scr = PTR_OFFSET_T(_img16, *(_imgOffset + num), uint16_t);
            uint16_t* tmp = scr;

            while (true){
                if (x1 >= 0 && x1 <= _width &&
                    y1 >= 0 && y1 <= _height){
                
                    scr = tmp + _width * y1 + x1;
                    *scr = col;
                }

                if (x1 == x2 && y1 == y2) break;

                int e2 = 2 * err;
                if (e2 > -dy){ err -= dy; x1 += sx; }
                if (e2 < dx){ err += dx; y1 += sy; }
            }
        } else {
            uint8_t* scr = PTR_OFFSET_T(_img8, *(_imgOffset + num), uint8_t);
            uint8_t* tmp = scr;            
            uint8_t color = (uint8_t)col;

            while (true){
                if (x1 >= 0 && x1 <= _width &&
                    y1 >= 0 && y1 <= _height){
                    uint8_t* scr = tmp + _width * y1 + x1;
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

void VGA_Sprite::lineAngle(int x, int y, int len, int angle, uint16_t col, int num){
    int x1 = _math.xLUT(x, y, len, angle);
    int y1 = _math.yLUT(x, y, len, angle);
    line(x, y, x1, y1, col);
}

void VGA_Sprite::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, int num){
    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

void VGA_Sprite::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, int num){
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

void VGA_Sprite::circle(int xc, int yc, int r, uint16_t col, int num){
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

void VGA_Sprite::fillCircle(int xc, int yc, int r, uint16_t col, int num){
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