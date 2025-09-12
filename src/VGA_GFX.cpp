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

void VGA_GFX::clsViewport(uint16_t col){
    int x1 = _vga.get_vX1();
    int y1 = _vga.get_vY1();
    int x2 = _vga.get_vX2();
    int y2 = _vga.get_vY2();

    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;
    int width = _vga.getScrWidth();

    if (_vga.getBpp() == 16){
        uint16_t* scr = _vga._buf16 + _vga._backBuff + *(_vga._fastY + y1) + x1;
        uint16_t* savePos = scr;
        int copyBytes = sizeX << 1;

        while (sizeX-- > 0) *scr++ = col;
        scr += width - x2 + x1 - 1;
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

        int skip = width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX;

        while (sizeX-- > 0) *scr++ = col;
        scr += skip;
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

void VGA_GFX::lineAngle(int x, int y, int len, int angle, uint16_t col){
    int x1 = _math.xLUT(x, y, len, angle);
    int y1 = _math.yLUT(x, y, len, angle);
    line(x, y, x1, y1, col);
}

void VGA_GFX::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

void VGA_GFX::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
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

void VGA_GFX::circle(int xc, int yc, int r, uint16_t col){
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

void VGA_GFX::fillCircle(int xc, int yc, int r, uint16_t col){
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

// --- Рисуем только контур многоугольника ---
void VGA_GFX::polygon(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (sides < 3) return;

    int* vx = new int[sides];
    int* vy = new int[sides];

    int angleStep = 360 / sides;

    // Вычисляем вершины через LUT
    for (int i = 0; i < sides; i++){
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, radius, angle);
        vy[i] = _math.yLUT(x, y, radius, angle);
    }

    // Соединяем вершины линиями
    for (int i = 0; i < sides; i++){
        int x1 = vx[i];
        int y1 = vy[i];
        int x2 = vx[(i + 1) % sides];
        int y2 = vy[(i + 1) % sides];

        line(x1, y1, x2, y2, col); // используем уже готовую функцию line
    }

    delete[] vx;
    delete[] vy;
}

// --- Закрашиваем многоугольник через hLine ---
void VGA_GFX::fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (sides < 3) return;

    int* vx = new int[sides];
    int* vy = new int[sides];

    int angleStep = 360 / sides;

    // Вычисляем вершины через LUT
    for (int i = 0; i < sides; i++){
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, radius, angle);
        vy[i] = _math.yLUT(x, y, radius, angle);
    }

    // Алгоритм scanline для закрашивания
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < sides; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        // собираем все пересечения текущей строки с ребрами
        int intersections[sides];
        int n = 0;

        for (int i = 0; i < sides; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % sides], y2 = vy[(i + 1) % sides];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортировка пересечений
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j]) std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные линии между парами пересечений
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

// --- Контур звезды ---
void VGA_GFX::star(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (sides < 2) return; // минимум 2 "луча"

    int points = sides * 2; // удвоенное количество вершин (внешние+внутренние)
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2; // чётные - внешний радиус, нечётные - внутренний
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, r, angle);
        vy[i] = _math.yLUT(x, y, r, angle);
    }

    // соединяем вершины линиями
    for (int i = 0; i < points; i++){
        int x1 = vx[i],     y1 = vy[i];
        int x2 = vx[(i+1)%points], y2 = vy[(i+1)%points];
        line(x1, y1, x2, y2, col);
    }

    delete[] vx;
    delete[] vy;
}

// --- Заполненная звезда ---
void VGA_GFX::fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (sides < 2) return;

    int points = sides * 2;
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2;
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, r, angle);
        vy[i] = _math.yLUT(x, y, r, angle);
    }

    // используем тот же scanline, что и в fillPolygon
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < points; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        int intersections[points];
        int n = 0;

        for (int i = 0; i < points; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % points], y2 = vy[(i + 1) % points];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортируем пересечения
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j])
                    std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные линии
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

// --- Контур звезды с разными радиусами ---
void VGA_GFX::star2(int x, int y, int radius1, int radius2, int sides, int rotation, uint16_t col){
    if (sides < 2) return; // минимум 2 луча

    int points = sides * 2;
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius1 : radius2; // чётные - внешний радиус, нечётные - внутренний
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, r, angle);
        vy[i] = _math.yLUT(x, y, r, angle);
    }

    // Соединяем вершины линиями
    for (int i = 0; i < points; i++){
        int x1 = vx[i], y1 = vy[i];
        int x2 = vx[(i + 1) % points], y2 = vy[(i + 1) % points];
        line(x1, y1, x2, y2, col);
    }

    delete[] vx;
    delete[] vy;
}

// --- Заполненная звезда с разными радиусами ---
void VGA_GFX::fillStar2(int x, int y, int radius1, int radius2, int sides, int rotation, uint16_t col){
    if (sides < 2) return;

    int points = sides * 2;
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius1 : radius2;
        int angle = rotation + i * angleStep;
        vx[i] = _math.xLUT(x, y, r, angle);
        vy[i] = _math.yLUT(x, y, r, angle);
    }

    // Алгоритм scanline
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < points; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        int intersections[points];
        int n = 0;

        for (int i = 0; i < points; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % points], y2 = vy[(i + 1) % points];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортировка пересечений
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j])
                    std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные отрезки
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

void VGA_GFX::ellipse(int xc, int yc, int rx, int ry, uint16_t col) {
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int twoRx2 = 2 * rx2;
    int twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;
    int px = 0;
    int py = twoRx2 * y;

    // Первая часть (dx/dy < 1)
    int p = round(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    // Вторая часть (dx/dy >= 1)
    p = round(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void VGA_GFX::fillEllipse(int xc, int yc, int rx, int ry, uint16_t col) {
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int twoRx2 = 2 * rx2;
    int twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;
    int px = 0;
    int py = twoRx2 * y;

    // Первая часть (dx/dy < 1)
    int p = round(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        hLine(xc - x, yc + y, xc + x, col);
        hLine(xc - x, yc - y, xc + x, col);

        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    // Вторая часть (dx/dy >= 1)
    p = round(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        hLine(xc - x, yc + y, xc + x, col);
        hLine(xc - x, yc - y, xc + x, col);

        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void VGA_GFX::circleHelper(int xc, int yc, int r, uint8_t corner, uint16_t col) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        if (corner & 1) { // верхний левый
            putPixel(xc - x, yc - y, col);
            putPixel(xc - y, yc - x, col);
        }
        if (corner & 2) { // верхний правый
            putPixel(xc + x, yc - y, col);
            putPixel(xc + y, yc - x, col);
        }
        if (corner & 4) { // нижний правый
            putPixel(xc + x, yc + y, col);
            putPixel(xc + y, yc + x, col);
        }
        if (corner & 8) { // нижний левый
            putPixel(xc - x, yc + y, col);
            putPixel(xc - y, yc + x, col);
        }

        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }
}

void VGA_GFX::fillCircleHelper(int xc, int yc, int r, uint8_t corner, int ybase, uint16_t col) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        if (corner & 1) { // верхний левый
            hLine(xc - x, yc - y, xc, col);
            hLine(xc - y, yc - x, xc, col);
        }
        if (corner & 2) { // верхний правый
            hLine(xc, yc - y, xc + x, col);
            hLine(xc, yc - x, xc + y, col);
        }
        if (corner & 4) { // нижний правый
            hLine(xc, yc + y, xc + x, col);
            hLine(xc, yc + x, xc + y, col);
        }
        if (corner & 8) { // нижний левый
            hLine(xc - x, yc + y, xc, col);
            hLine(xc - y, yc + x, xc, col);
        }

        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }
}

void VGA_GFX::roundRect(int x1, int y1, int x2, int y2, int r, uint16_t col) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Боковые линии
    vLine(x1, y1 + r, y2 - r, col);
    vLine(x2, y1 + r, y2 - r, col);
    hLine(x1 + r, y1, x2 - r, col);
    hLine(x1 + r, y2, x2 - r, col);

    // Углы
    int xc, yc;
    xc = x1 + r; yc = y1 + r;
    circleHelper(xc, yc, r, 1, col);  // верхний левый
    xc = x2 - r; yc = y1 + r;
    circleHelper(xc, yc, r, 2, col);  // верхний правый
    xc = x2 - r; yc = y2 - r;
    circleHelper(xc, yc, r, 4, col);  // нижний правый
    xc = x1 + r; yc = y2 - r;
    circleHelper(xc, yc, r, 8, col);  // нижний левый
}

void VGA_GFX::fillRoundRect(int x1, int y1, int x2, int y2, int r, uint16_t col) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Центральная часть
    fillRect(x1 + r, y1, x2 - r, y2, col);

    // Левые и правые полоски
    fillRect(x1, y1 + r, x1 + r, y2 - r, col);
    fillRect(x2 - r, y1 + r, x2, y2 - r, col);

    // Углы
    int xc, yc;
    xc = x1 + r; yc = y1 + r;
    fillCircleHelper(xc, yc, r, 1, y1, col); // верхний левый
    xc = x2 - r; yc = y1 + r;
    fillCircleHelper(xc, yc, r, 2, y1, col); // верхний правый
    xc = x2 - r; yc = y2 - r;
    fillCircleHelper(xc, yc, r, 4, y2, col); // нижний правый
    xc = x1 + r; yc = y2 - r;
    fillCircleHelper(xc, yc, r, 8, y2, col); // нижний левый
}

void VGA_GFX::arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col) {
    if (angle1 > angle2) std::swap(angle1, angle2);
    angle1 %= 360;
    angle2 %= 360;

    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    auto inAngle = [&](int angle) {
        if (angle1 <= angle2) return (angle >= angle1 && angle <= angle2);
        return (angle >= angle1 || angle <= angle2); // переход через 360
    };

    auto plot = [&](int dx, int dy) {
        // вычисляем угол точки относительно центра
        int angle = (int)(atan2((float)dy, (float)dx) * 180.0 / M_PI);
        if (angle < 0) angle += 360;
        if (inAngle(angle)) putPixel(xc + dx, yc + dy, col);
    };

    while (y >= x) {
        plot(x, y);
        plot(-x, y);
        plot(x, -y);
        plot(-x, -y);
        plot(y, x);
        plot(-y, x);
        plot(y, -x);
        plot(-y, -x);

        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }
}

void VGA_GFX::fillArc(int xc, int yc, int r, int angle1, int angle2, uint16_t col) {
    if (angle1 > angle2) std::swap(angle1, angle2);
    angle1 %= 360;
    angle2 %= 360;

    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    auto inAngle = [&](int angle) {
        if (angle1 <= angle2) return (angle >= angle1 && angle <= angle2);
        return (angle >= angle1 || angle <= angle2); // переход через 360
    };

    auto drawLine = [&](int dx, int dy) {
        int angle = (int)(atan2((float)dy, (float)dx) * 180.0 / M_PI);
        if (angle < 0) angle += 360;
        if (inAngle(angle)) {
            hLine(xc, yc, xc + dx, col);
            hLine(xc, yc, xc + dx, col);
        }
    };

    while (y >= x) {
        drawLine(x, y);
        drawLine(-x, y);
        drawLine(x, -y);
        drawLine(-x, -y);
        drawLine(y, x);
        drawLine(-y, x);
        drawLine(y, -x);
        drawLine(-y, -x);

        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }
}

void VGA_GFX::bezier(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col) {
    const int STEPS = 100; // чем больше, тем плавнее
    int prevX = x0;
    int prevY = y0;

    for (int i = 1; i <= STEPS; i++) {
        float t = (float)i / STEPS;
        float u = 1 - t;

        // формула квадратичной Безье
        int x = (int)(u*u*x0 + 2*u*t*x1 + t*t*x2);
        int y = (int)(u*u*y0 + 2*u*t*y1 + t*t*y2);

        line(prevX, prevY, x, y, col);
        prevX = x;
        prevY = y;
    }
}

void VGA_GFX::bezierCubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col) {
    const int STEPS = 100;
    int prevX = x0;
    int prevY = y0;

    for (int i = 1; i <= STEPS; i++) {
        float t = (float)i / STEPS;
        float u = 1 - t;

        // формула кубической Безье
        int x = (int)(u*u*u*x0 + 3*u*u*t*x1 + 3*u*t*t*x2 + t*t*t*x3);
        int y = (int)(u*u*u*y0 + 3*u*u*t*y1 + 3*u*t*t*y2 + t*t*t*y3);

        line(prevX, prevY, x, y, col);
        prevX = x;
        prevY = y;
    }
}

void VGA_GFX::spiral(int xc, int yc, int r, int turns, uint16_t col) {
    const int STEPS = 360 * turns;
    float radiusStep = (float)r / STEPS;

    int prevX = xc;
    int prevY = yc;

    for (int i = 0; i <= STEPS; i++) {
        float angle = i * M_PI / 180.0f; // в радианы
        float radius = i * radiusStep;

        int x = xc + (int)(radius * cos(angle));
        int y = yc + (int)(radius * sin(angle));

        if (i > 0) line(prevX, prevY, x, y, col);

        prevX = x;
        prevY = y;
    }
}

void VGA_GFX::wave(int x, int y, int len, int amp, int freq, uint16_t col) {
    const int STEPS = len;
    int prevX = x;
    int prevY = y;

    for (int i = 1; i <= STEPS; i++) {
        int xi = x + i;
        float angle = (float)i * freq * M_PI / 180.0f; // угол в радианах
        int yi = y + (int)(amp * sin(angle));

        line(prevX, prevY, xi, yi, col);

        prevX = xi;
        prevY = yi;
    }
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