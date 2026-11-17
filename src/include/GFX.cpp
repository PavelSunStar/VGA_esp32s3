#include "../VGA_esp32.h"
#include "../colorConverter.h"

void VGA_esp32::testRGB(){
    auto& s = _s;

    if (!s.inited) return;

    int w = s.width;
    int h = s.height;

    cls(0);

    // ========================================================
    // Разбиваем экран по высоте
    // ========================================================

    int y0 = 0;
    int y1 = h * 12 / 100;
    int y2 = h * 25 / 100;
    int y3 = h * 37 / 100;
    int y4 = h * 58 / 100;
    int y5 = h * 79 / 100;
    int y6 = h - 1;

    if (s.bpp == ColorDepth::BPP_16){
        // ====================================================
        // RED — 32 уровня
        // ====================================================

        for (int i = 0; i < 32; i++){
            int x0 = i * w / 32;
            int x1 = (i + 1) * w / 32 - 1;
            fillRect(x0, y0, x1, y1 - 1, i << 11);
        }

        // ====================================================
        // GREEN — 64 уровня
        // ====================================================

        for (int i = 0; i < 64; i++){
            int x0 = i * w / 64;
            int x1 = (i + 1) * w / 64 - 1;
            fillRect(x0, y1, x1, y2 - 1, i << 5);
        }

        // ====================================================
        // BLUE — 32 уровня
        // ====================================================

        for (int i = 0; i < 32; i++){
            int x0 = i * w / 32;
            int x1 = (i + 1) * w / 32 - 1;
            fillRect(x0, y2, x1, y3 - 1, i);
        }

        // ====================================================
        // Основные цвета RGB565
        // ====================================================

        const uint16_t colors[] = {
            0x0000, 0xF800, 0x07E0, 0x001F,
            0xFFE0, 0xF81F, 0x07FF, 0xFFFF
        };

        for (int i = 0; i < 8; i++){
            int x0 = i * w / 8;
            int x1 = (i + 1) * w / 8 - 1;
            fillRect(x0, y3, x1, y4 - 1, colors[i]);
        }

        // ====================================================
        // RGB градиент
        // ====================================================

        for (int x = 0; x < w; x++){
            uint8_t r, g, b;
            int p = x * 3 * 255 / w;
            int part = p / 255;
            int v = p % 255;

            if (part == 0){
                r = 255 - v; g = v; b = 0;
            } else if (part == 1){
                r = 0; g = 255 - v; b = v;
            } else {
                r = v; g = 0; b = 255 - v;
            }

            uint16_t color = ((r & 0xF8) << 8) |
                             ((g & 0xFC) << 3) |
                             (b >> 3);

            vLine(x, y4, y5 - 1, color);
        }

        // ====================================================
        // Серый RGB565 — 32 уровня
        // ====================================================

        for (int x = 0; x < w; x++){
            int level = x * 32 / w;
            if (level > 31) level = 31;

            uint16_t r = level;
            uint16_t g = level * 63 / 31;
            uint16_t b = level;

            uint16_t color = (r << 11) | (g << 5) | b;

            vLine(x, y5, y6, color);
        }
    } else {
        // ====================================================
        // RGB332
        // RRR GGG BB
        // ====================================================

        // RED — 8 уровней
        for (int i = 0; i < 8; i++){
            int x0 = i * w / 8;
            int x1 = (i + 1) * w / 8 - 1;
            fillRect(x0, y0, x1, y1 - 1, i << 5);
        }

        // GREEN — 8 уровней
        for (int i = 0; i < 8; i++){
            int x0 = i * w / 8;
            int x1 = (i + 1) * w / 8 - 1;
            fillRect(x0, y1, x1, y2 - 1, i << 2);
        }

        // BLUE — 4 уровня
        for (int i = 0; i < 4; i++){
            int x0 = i * w / 4;
            int x1 = (i + 1) * w / 4 - 1;
            fillRect(x0, y2, x1, y3 - 1, i);
        }

        // ====================================================
        // Основные цвета RGB332
        // ====================================================

        const uint8_t colors[] = {
            0x00,       // Black
            0xE0,       // Red
            0x1C,       // Green
            0x03,       // Blue
            0xFC,       // Yellow
            0xE3,       // Magenta
            0x1F,       // Cyan
            0xFF        // White
        };

        for (int i = 0; i < 8; i++){
            int x0 = i * w / 8;
            int x1 = (i + 1) * w / 8 - 1;
            fillRect(x0, y3, x1, y4 - 1, colors[i]);
        }

        // ====================================================
        // RGB градиент
        // ====================================================

        for (int x = 0; x < w; x++){
            uint8_t r, g, b;
            int p = x * 3 * 255 / w;
            int part = p / 255;
            int v = p % 255;

            if (part == 0){
                r = 255 - v; g = v; b = 0;
            } else if (part == 1){
                r = 0; g = 255 - v; b = v;
            } else {
                r = v; g = 0; b = 255 - v;
            }

            uint8_t color = (r & 0xE0) |
                            ((g & 0xE0) >> 3) |
                            (b >> 6);

            vLine(x, y4, y5 - 1, color);
        }

        // ====================================================
        // Серый градиент RGB332 — 4 нейтральных уровня
        // ====================================================

        for (int x = 0; x < w; x++){
            int level = x * 4 / w;
            if (level > 3) level = 3;

            uint8_t r = level * 7 / 3;
            uint8_t g = level * 7 / 3;
            uint8_t b = level;

            uint8_t color = (r << 5) | (g << 2) | b;

            vLine(x, y5, y6, color);
        }
    }
}

/*
void VGA_esp32::cls(uint16_t col){
    uint32_t color = (uint32_t)col;
    if (_s.bpp == ColorDepth::BPP_8) color |= color << 8;
    color |= color << 16;

    int pixels = _s.lineSize >> 2;
    uint32_t* buf = (uint32_t*)_s.tmpBuf;
    uint32_t* savePos = buf;

    while (pixels-- > 0) *buf++ = color;
    memcpy(buf, savePos, _sp.copyBytes);
    buf += (_sp.copyBytes >> 2);
    memcpy(buf, savePos, _sp.copyBytes2x);

    int lines = _s.height >> 2;
    uint8_t* dst = (uint8_t*)_s.fb0;
    while (lines-- > 0){
        memcpy(dst, _s.tmpBuf, _sp.copyBytes4x);
        dst += _sp.copyBytes4x;
    }
}
*/

void VGA_esp32::cls(uint16_t col){
    auto& s = _s;
    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        memset(s.bLine8[0], (uint8_t)col, s.size);
    } else {
        uint16_t* scr = (uint16_t*)s.bLine16[0];

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, col , s.fullSize);
        } else {
            int size = 0;
            uint16_t* cpy = scr;
            while (size++ < s.width) *scr++ = col;

            int dummy = 1; 
            int lines = s.maxY;  
            int copyBytes = s.lineSize;
            int offset = s.width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =s.lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }
    }
}

uint16_t VGA_esp32::getPixel(int x, int y){
    auto& s = _s;

    if (!s.inited) return 0;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return 0;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        return (uint16_t)s.bLine8[y][x];
    } else {
        return s.bLine16[y][x];
    }   
}

void VGA_esp32::putPixel(int x, int y, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        s.bLine8[y][x] = (uint8_t)col;
    } else {
        s.bLine16[y][x] = col;
    }
}

void VGA_esp32::hLine(int x0, int y, int x1, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;
    if (y < s.y0 || y > s.y1) return;

    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return;

    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);

    int count = x1 - x0 + 1;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        memset(&s.bLine8[y][x0], (uint8_t)col, count);
    } else {
        uint16_t* scr = &s.bLine16[y][x0];

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)col, count << 1);
        } else {
            while (count-- > 0)
                *scr++ = col;       // <-- именно uint16_t col
        }
    }
}

void VGA_esp32::hLineLen(int x, int y, int len, uint16_t col){
    if (len <= 0) return;
    hLine(x, y, x + len - 1, col);
}

void VGA_esp32::vLine(int x, int y0, int y1, uint16_t col){
    auto& s = _s;
    if (!s.inited) return;

    if (x < s.x0 || x > s.x1) return;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    if (s.usePal || s.bpp == ColorDepth::BPP_8)    {
        uint8_t color = (uint8_t)col;
        uint8_t* scr = &s.bLine8[y0][x];

        while (y0++ <= y1){
            *scr = color;
            scr += s.width;
        }
    } else {
        uint16_t* scr = &s.bLine16[y0][x];

        while (y0++ <= y1){
            *scr = col;
            scr += s.width;
        }
    }
}

void VGA_esp32::vLineLen(int x, int y, int len, uint16_t col){
    if (len <= 0) return;
    vLine(x, y, y + len - 1, col);
}

void VGA_esp32::rect(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 || y1 < s.y0) return;

    if (x0 == x1 && y0 == y1){
        putPixel(x0, y0, col);
        return;
    }

    if (x0 == x1){
        vLine(x0, y0, y1, col);
        return;
    }

    if (y0 == y1){
        hLine(x0, y0, x1, col);
        return;
    }

    if (x0 >= s.x0 && y0 >= s.y0 && x1 <= s.x1 && y1 <= s.y1){
        int sizeX = x1 - x0 + 1;
        int sizeY = y1 - y0 - 1;        
        int skip1 = x1 - x0; 
        int skip2 = s.width - x1 + x0;        

        if (s.usePal || s.bpp == ColorDepth::BPP_8){
            uint8_t* src = &s.bLine8[y0][x0];
            uint8_t color = (uint8_t)col;

            memset(src, color, sizeX); src += s.width;
            while (sizeY-- > 0){
                *src = color; src += skip1;
                *src = color; src += skip2;
            }
            memset(src, color, sizeX);
        } else {
            uint16_t* src = &s.bLine16[y0][x0];
            uint16_t* cpy = src;
            int lineSize = sizeX << 1;
            
            if ((uint8_t)col == (uint8_t)(col >> 8)){
                memset(src, (uint8_t)col, lineSize);
                src += s.width;
            } else {
                while (sizeX-- > 0) *src++ = col; 
                src += skip2 - 1;       
            }

            while (sizeY-- > 0){
                *src = col; src += skip1;
                *src = col; src += skip2;
            }

            memcpy(src, cpy, lineSize);            
        }

        return;
    }

    hLine(x0, y0, x1, col);
    hLine(x0, y1, x1, col);
    vLine(x0, y0 + 1, y1 - 1, col);
    vLine(x1, y0 + 1, y1 - 1, col);
}

void VGA_esp32::fillRect(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 || y1 < s.y0) return;

    if (x0 == x1 && y0 == y1){
        putPixel(x0, y0, col);
        return;
    }

    if (x0 == x1){
        vLine(x0, y0, y1, col);
        return;
    }

    if (y0 == y1){
        hLine(x0, y0, x1, col);
        return;
    }

    x0 = std::max(x0, s.x0);
    x1 = std::min(x1, s.x1);
    y0 = std::max(y0, s.y0);
    y1 = std::min(y1, s.y1);

    // Весь framebuffer
    if (x0 == 0 && y0 == 0 && x1 == s.maxX && y1 == s.maxY){
        cls(col);
        return;
    }

    int sizeX = x1 - x0 + 1;
    int sizeY = y1 - y0 + 1;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        uint8_t* src = &s.bLine8[y0][x0];
        uint8_t color = (uint8_t)col;

        memset(src, color, sizeX);

        uint8_t* dst = src + s.width;
        while (--sizeY > 0){
            memcpy(dst, src, sizeX);
            dst += s.width;
        }

    } else {
        uint16_t* dst = &s.bLine16[y0][x0];
        int lineSize = sizeX << 1;

        uint16_t line[640];

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(line, (uint8_t)col, lineSize);
        } else {
            for (int x = 0; x < sizeX; x++) line[x] = col;
        }

        while (sizeY-- > 0){
            memcpy(dst, line, lineSize);
            dst += s.width;
        }
    }
}

void VGA_esp32::line(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;

    // Горизонтальная
    if (y0 == y1){
        hLine(x0, y0, x1, col);
        return;
    }

    // Вертикальная
    if (x0 == x1){
        vLine(x0, y0, y1, col);
        return;
    }

    // Если линия полностью вне экрана
    if ((x0 < s.x0 && x1 < s.x0) ||
        (x0 > s.x1 && x1 > s.x1) ||
        (y0 < s.y0 && y1 < s.y0) ||
        (y0 > s.y1 && y1 > s.y1)) return;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        uint8_t color = (uint8_t)col;

        while (true){
            if (x0 >= s.x0 && x0 <= s.x1 &&
                y0 >= s.y0 && y0 <= s.y1)
                s.bLine8[y0][x0] = color;

            if (x0 == x1 && y0 == y1) break;

            int e2 = err << 1;

            if (e2 > -dy){
                err -= dy;
                x0 += sx;
            }

            if (e2 < dx){
                err += dx;
                y0 += sy;
            }
        }

    } else {
        while (true){
            if (x0 >= s.x0 && x0 <= s.x1 &&
                y0 >= s.y0 && y0 <= s.y1)
                s.bLine16[y0][x0] = col;

            if (x0 == x1 && y0 == y1) break;

            int e2 = err << 1;

            if (e2 > -dy){
                err -= dy;
                x0 += sx;
            }

            if (e2 < dx){
                err += dx;
                y0 += sy;
            }
        }
    }
}

void VGA_esp32::triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col){
    line(x0, y0, x1, y1, col);
    line(x1, y1, x2, y2, col);
    line(x2, y2, x0, y0, col);
}

void VGA_esp32::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col){
    auto& s = _s;
    if (!s.inited) return;

    if (y0 > y1){
        std::swap(y0, y1);
        std::swap(x0, x1);
    }
    if (y1 > y2){
        std::swap(y1, y2);
        std::swap(x1, x2);
    }
    if (y0 > y1){
        std::swap(y0, y1);
        std::swap(x0, x1);
    }

    // Вырожденный горизонтальный треугольник
    if (y0 == y2){
        int minX = std::min(x0, std::min(x1, x2));
        int maxX = std::max(x0, std::max(x1, x2));
        hLine(minX, y0, maxX, col);
        return;
    }

    // Длинная сторона 0 -> 2
    int32_t xb = x0 << 16;
    int32_t stepB = ((int32_t)(x2 - x0) << 16) / (y2 - y0);

    // Верхняя короткая сторона 0 -> 1
    if (y1 > y0){
        int32_t xa = x0 << 16;
        int32_t stepA = ((int32_t)(x1 - x0) << 16) / (y1 - y0);

        for (int y = y0; y < y1; y++){
            int a = xa >> 16;
            int b = xb >> 16;

            if (a > b) std::swap(a, b);
            hLine(a, y, b, col);

            xa += stepA;
            xb += stepB;
        }
    }

    // Нижняя короткая сторона 1 -> 2
    if (y2 >= y1){
        int32_t xa = x1 << 16;
        int32_t stepA = (y2 != y1)
                      ? ((int32_t)(x2 - x1) << 16) / (y2 - y1)
                      : 0;

        // xb должен соответствовать координате y1
        xb = (x0 << 16) + stepB * (y1 - y0);

        for (int y = y1; y <= y2; y++){
            int a = xa >> 16;
            int b = xb >> 16;

            if (a > b) std::swap(a, b);
            hLine(a, y, b, col);

            xa += stepA;
            xb += stepB;
        }
    }
}

void VGA_esp32::circle(int xc, int yc, int r, uint16_t col){
    auto& s = _s;

    if (!s.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    // Круг полностью вне clipping area
    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    int x = 0;
    int y = r;
    int d = 1 - r;

    // Быстрый путь: весь круг внутри clipping
    if (xc - r >= s.x0 && xc + r <= s.x1 &&
        yc - r >= s.y0 && yc + r <= s.y1){

        if (s.usePal || s.bpp == ColorDepth::BPP_8){
            uint8_t color = (uint8_t)col;

            while (x <= y){
                s.bLine8[yc + y][xc + x] = color;
                s.bLine8[yc + y][xc - x] = color;
                s.bLine8[yc - y][xc + x] = color;
                s.bLine8[yc - y][xc - x] = color;

                s.bLine8[yc + x][xc + y] = color;
                s.bLine8[yc + x][xc - y] = color;
                s.bLine8[yc - x][xc + y] = color;
                s.bLine8[yc - x][xc - y] = color;

                x++;

                if (d < 0){
                    d += (x << 1) + 1;
                } else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }

        } else {
            while (x <= y){
                s.bLine16[yc + y][xc + x] = col;
                s.bLine16[yc + y][xc - x] = col;
                s.bLine16[yc - y][xc + x] = col;
                s.bLine16[yc - y][xc - x] = col;

                s.bLine16[yc + x][xc + y] = col;
                s.bLine16[yc + x][xc - y] = col;
                s.bLine16[yc - x][xc + y] = col;
                s.bLine16[yc - x][xc - y] = col;

                x++;

                if (d < 0){
                    d += (x << 1) + 1;
                } else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }
        }

        return;
    }

    // Частично за пределами clipping
    while (x <= y){
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        putPixel(xc + y, yc + x, col);
        putPixel(xc - y, yc + x, col);
        putPixel(xc + y, yc - x, col);
        putPixel(xc - y, yc - x, col);

        x++;

        if (d < 0){
            d += (x << 1) + 1;
        } else {
            y--;
            d += ((x - y) << 1) + 1;
        }
    }
}

void VGA_esp32::fillCircle(int xc, int yc, int r, uint16_t col){
    auto& s = _s;

    if (!s.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    hLine(xc - r, yc, xc + r, col);

    while (y >= x){
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
            d += ((x - y) << 2) + 10;
        } else {
            d += (x << 2) + 6;
        }
    }
}

void VGA_esp32::thickLine(int x0, int y0, int x1, int y1, int width, uint16_t col){
    auto& s = _s;

    if (!s.inited || width <= 0) return;

    if (width == 1){
        line(x0, y0, x1, y1, col);
        return;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    if (dx == 0 && dy == 0){
        fillCircle(x0, y0, width >> 1, col);
        return;
    }

    float len = sqrtf((float)dx * dx + (float)dy * dy);
    float half = width * 0.5f;

    int ox = (int)roundf(-dy * half / len);
    int oy = (int)roundf( dx * half / len);

    int ax = x0 + ox;
    int ay = y0 + oy;

    int bx = x1 + ox;
    int by = y1 + oy;

    int cx = x1 - ox;
    int cy = y1 - oy;

    int dx2 = x0 - ox;
    int dy2 = y0 - oy;

    fillTriangle(ax, ay, bx, by, cx, cy, col);
    fillTriangle(ax, ay, cx, cy, dx2, dy2, col);
}

void VGA_esp32::roundRect(int x0, int y0, int x1, int y1, int r, uint16_t col){
    auto& s = _s;
    if (!s.inited) return;

    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 || y1 < s.y0) return;

    int w = x1 - x0;
    int h = y1 - y0;

    if (r <= 0){
        rect(x0, y0, x1, y1, col);
        return;
    }

    int maxR = std::min(w, h) >> 1;
    if (r > maxR) r = maxR;

    if (r == 0){
        rect(x0, y0, x1, y1, col);
        return;
    }

    hLine(x0 + r, y0, x1 - r, col);
    hLine(x0 + r, y1, x1 - r, col);
    vLine(x0, y0 + r, y1 - r, col);
    vLine(x1, y0 + r, y1 - r, col);

    int xl = x0 + r;
    int xr = x1 - r;
    int yt = y0 + r;
    int yb = y1 - r;

    int x = 0;
    int y = r;
    int d = 1 - r;

    bool inside = x0 >= s.x0 && y0 >= s.y0 &&
                  x1 <= s.x1 && y1 <= s.y1;

    if (inside){
        if (s.usePal || s.bpp == ColorDepth::BPP_8){
            uint8_t color = (uint8_t)col;

            while (x <= y){
                s.bLine8[yt - x][xl - y] = color;
                s.bLine8[yt - y][xl - x] = color;

                s.bLine8[yt - x][xr + y] = color;
                s.bLine8[yt - y][xr + x] = color;

                s.bLine8[yb + x][xl - y] = color;
                s.bLine8[yb + y][xl - x] = color;

                s.bLine8[yb + x][xr + y] = color;
                s.bLine8[yb + y][xr + x] = color;

                x++;

                if (d < 0) d += (x << 1) + 1;
                else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }
        } else {
            while (x <= y){
                s.bLine16[yt - x][xl - y] = col;
                s.bLine16[yt - y][xl - x] = col;

                s.bLine16[yt - x][xr + y] = col;
                s.bLine16[yt - y][xr + x] = col;

                s.bLine16[yb + x][xl - y] = col;
                s.bLine16[yb + y][xl - x] = col;

                s.bLine16[yb + x][xr + y] = col;
                s.bLine16[yb + y][xr + x] = col;

                x++;

                if (d < 0) d += (x << 1) + 1;
                else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }
        }

        return;
    }

    // Частично за пределами clipping
    while (x <= y){
        putPixel(xl - y, yt - x, col);
        putPixel(xl - x, yt - y, col);

        putPixel(xr + y, yt - x, col);
        putPixel(xr + x, yt - y, col);

        putPixel(xl - y, yb + x, col);
        putPixel(xl - x, yb + y, col);

        putPixel(xr + y, yb + x, col);
        putPixel(xr + x, yb + y, col);

        x++;

        if (d < 0) d += (x << 1) + 1;
        else {
            y--;
            d += ((x - y) << 1) + 1;
        }
    }
}

void VGA_esp32::fillRoundRect(int x0, int y0, int x1, int y1, int r, uint16_t col){
    auto& s = _s;
    if (!s.inited) return;

    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 || y1 < s.y0) return;

    int w = x1 - x0;
    int h = y1 - y0;

    if (r <= 0){
        fillRect(x0, y0, x1, y1, col);
        return;
    }

    int maxR = std::min(w, h) >> 1;
    if (r > maxR) r = maxR;

    if (r == 0){
        fillRect(x0, y0, x1, y1, col);
        return;
    }

    int xl = x0 + r;
    int xr = x1 - r;

    // Центральная часть
    fillRect(x0, y0 + r, x1, y1 - r, col);

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    while (y >= x){
        // Строки ближе к центру закругления
        if (x > 0){
            hLine(xl - y, y0 + r - x, xr + y, col);
            hLine(xl - y, y1 - r + x, xr + y, col);
        }

        // Строки ближе к верхнему/нижнему краю
        if (y > x){
            hLine(xl - x, y0 + r - y, xr + x, col);
            hLine(xl - x, y1 - r + y, xr + x, col);
        }

        x++;

        if (d > 0){
            y--;
            d += ((x - y) << 2) + 10;
        } else {
            d += (x << 2) + 6;
        }
    }
}

void VGA_esp32::ellipse(int xc, int yc, int rx, int ry, uint16_t col){
    auto& s = _s;
    if (!s.inited || rx < 0 || ry < 0) return;

    if (rx == 0 && ry == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (rx == 0){
        vLine(xc, yc - ry, yc + ry, col);
        return;
    }

    if (ry == 0){
        hLine(xc - rx, yc, xc + rx, col);
        return;
    }

    // Полностью вне clipping
    if (xc + rx < s.x0 || xc - rx > s.x1 ||
        yc + ry < s.y0 || yc - ry > s.y1) return;

    int64_t rx2 = (int64_t)rx * rx;
    int64_t ry2 = (int64_t)ry * ry;

    int x = 0;
    int y = ry;

    int64_t px = 0;
    int64_t py = 2 * rx2 * y;

    int64_t p = ry2 - rx2 * ry + (rx2 >> 2);

    bool inside =
        xc - rx >= s.x0 && xc + rx <= s.x1 &&
        yc - ry >= s.y0 && yc + ry <= s.y1;

    if (inside){
        if (s.usePal || s.bpp == ColorDepth::BPP_8){
            uint8_t color = (uint8_t)col;

            // Region 1
            while (px < py){
                s.bLine8[yc + y][xc + x] = color;
                s.bLine8[yc + y][xc - x] = color;
                s.bLine8[yc - y][xc + x] = color;
                s.bLine8[yc - y][xc - x] = color;

                x++;
                px += 2 * ry2;

                if (p < 0){
                    p += ry2 + px;
                } else {
                    y--;
                    py -= 2 * rx2;
                    p += ry2 + px - py;
                }
            }

            // Region 2
            p = ry2 * (2LL * x + 1) * (2LL * x + 1) +
                4LL * rx2 * (y - 1) * (y - 1) -
                4LL * rx2 * ry2;

            while (y >= 0){
                s.bLine8[yc + y][xc + x] = color;
                s.bLine8[yc + y][xc - x] = color;
                s.bLine8[yc - y][xc + x] = color;
                s.bLine8[yc - y][xc - x] = color;

                y--;

                if (p > 0){
                    p += 4LL * rx2 * (1 - 2LL * y);
                } else {
                    x++;
                    p += 8LL * ry2 * x +
                         4LL * rx2 * (1 - 2LL * y);
                }
            }
        } else {
            // Region 1
            while (px < py){
                s.bLine16[yc + y][xc + x] = col;
                s.bLine16[yc + y][xc - x] = col;
                s.bLine16[yc - y][xc + x] = col;
                s.bLine16[yc - y][xc - x] = col;

                x++;
                px += 2 * ry2;

                if (p < 0){
                    p += ry2 + px;
                } else {
                    y--;
                    py -= 2 * rx2;
                    p += ry2 + px - py;
                }
            }

            // Region 2
            p = ry2 * (2LL * x + 1) * (2LL * x + 1) +
                4LL * rx2 * (y - 1) * (y - 1) -
                4LL * rx2 * ry2;

            while (y >= 0){
                s.bLine16[yc + y][xc + x] = col;
                s.bLine16[yc + y][xc - x] = col;
                s.bLine16[yc - y][xc + x] = col;
                s.bLine16[yc - y][xc - x] = col;

                y--;

                if (p > 0){
                    p += 4LL * rx2 * (1 - 2LL * y);
                } else {
                    x++;
                    p += 8LL * ry2 * x +
                         4LL * rx2 * (1 - 2LL * y);
                }
            }
        }

        return;
    }

    // Частично выходит за clipping
    while (px < py){
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        x++;
        px += 2 * ry2;

        if (p < 0){
            p += ry2 + px;
        } else {
            y--;
            py -= 2 * rx2;
            p += ry2 + px - py;
        }
    }

    p = ry2 * (2LL * x + 1) * (2LL * x + 1) +
        4LL * rx2 * (y - 1) * (y - 1) -
        4LL * rx2 * ry2;

    while (y >= 0){
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        y--;

        if (p > 0){
            p += 4LL * rx2 * (1 - 2LL * y);
        } else {
            x++;
            p += 8LL * ry2 * x +
                 4LL * rx2 * (1 - 2LL * y);
        }
    }
}

void VGA_esp32::fillEllipse(int xc, int yc, int rx, int ry, uint16_t col){
    auto& s = _s;
    if (!s.inited || rx < 0 || ry < 0) return;

    if (rx == 0 && ry == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (rx == 0){
        vLine(xc, yc - ry, yc + ry, col);
        return;
    }

    if (ry == 0){
        hLine(xc - rx, yc, xc + rx, col);
        return;
    }

    if (xc + rx < s.x0 || xc - rx > s.x1 ||
        yc + ry < s.y0 || yc - ry > s.y1) return;

    int64_t rx2 = (int64_t)rx * rx;
    int64_t ry2 = (int64_t)ry * ry;
    int64_t rr  = rx2 * ry2;

    int x = rx;

    // Центральная строка чуть короче:
    // убираем одиночные крайние точки слева/справа
    hLine(xc - rx + 1, yc, xc + rx - 1, col);

    for (int y = 1; y < ry; y++){
        int64_t yy = (int64_t)y * y;

        while (x > 0 &&
              (int64_t)x * x * ry2 + yy * rx2 > rr){
            x--;
        }

        if (x <= 0) continue;

        hLine(xc - x, yc - y, xc + x, col);
        hLine(xc - x, yc + y, xc + x, col);
    }

    // y = ±ry специально не рисуем:
    // убираем одиночные точки сверху и снизу
}

void VGA_esp32::arc(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col){
    auto& s = _s;
    if (!s.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    startAngle %= 360;
    endAngle %= 360;

    if (startAngle < 0) startAngle += 360;
    if (endAngle < 0) endAngle += 360;

    // Если одинаковые углы — считаем полным кругом
    bool fullCircle = (startAngle == endAngle);

    constexpr float DEG = 0.01745329251994329577f;

    float a0 = startAngle * DEG;
    float a1 = endAngle * DEG;

    // Масштабируем векторы, чтобы дальше работать с int32
    constexpr int SCALE = 32767;

    int32_t sx = (int32_t)roundf(cosf(a0) * SCALE);
    int32_t sy = (int32_t)roundf(sinf(a0) * SCALE);

    int32_t ex = (int32_t)roundf(cosf(a1) * SCALE);
    int32_t ey = (int32_t)roundf(sinf(a1) * SCALE);

    bool wrap = startAngle > endAngle;

    auto inArc = [&](int px, int py) -> bool {
        if (fullCircle) return true;

        int32_t dx = px - xc;
        int32_t dy = py - yc;

        int32_t crossStart = sx * dy - sy * dx;
        int32_t crossEnd   = ex * dy - ey * dx;

        if (!wrap)
            return crossStart >= 0 && crossEnd <= 0;

        return crossStart >= 0 || crossEnd <= 0;
    };

    bool inside =
        xc - r >= s.x0 && xc + r <= s.x1 &&
        yc - r >= s.y0 && yc + r <= s.y1;

    int x = 0;
    int y = r;
    int d = 1 - r;

    if (inside){
        if (s.usePal || s.bpp == ColorDepth::BPP_8){
            uint8_t color = (uint8_t)col;

            auto draw = [&](int px, int py){
                if (inArc(px, py))
                    s.bLine8[py][px] = color;
            };

            while (x <= y){
                draw(xc + x, yc + y);
                draw(xc - x, yc + y);
                draw(xc + x, yc - y);
                draw(xc - x, yc - y);

                draw(xc + y, yc + x);
                draw(xc - y, yc + x);
                draw(xc + y, yc - x);
                draw(xc - y, yc - x);

                x++;

                if (d < 0){
                    d += (x << 1) + 1;
                } else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }
        } else {
            auto draw = [&](int px, int py){
                if (inArc(px, py))
                    s.bLine16[py][px] = col;
            };

            while (x <= y){
                draw(xc + x, yc + y);
                draw(xc - x, yc + y);
                draw(xc + x, yc - y);
                draw(xc - x, yc - y);

                draw(xc + y, yc + x);
                draw(xc - y, yc + x);
                draw(xc + y, yc - x);
                draw(xc - y, yc - x);

                x++;

                if (d < 0){
                    d += (x << 1) + 1;
                } else {
                    y--;
                    d += ((x - y) << 1) + 1;
                }
            }
        }

        return;
    }

    // Частично за clipping
    auto draw = [&](int px, int py){
        if (inArc(px, py))
            putPixel(px, py, col);
    };

    while (x <= y){
        draw(xc + x, yc + y);
        draw(xc - x, yc + y);
        draw(xc + x, yc - y);
        draw(xc - x, yc - y);

        draw(xc + y, yc + x);
        draw(xc - y, yc + x);
        draw(xc + y, yc - x);
        draw(xc - y, yc - x);

        x++;

        if (d < 0){
            d += (x << 1) + 1;
        } else {
            y--;
            d += ((x - y) << 1) + 1;
        }
    }
}

void VGA_esp32::pie(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col){
    auto& s = _s;
    if (!s.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    // Полностью вне clipping
    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    startAngle %= 360;
    endAngle %= 360;

    if (startAngle < 0) startAngle += 360;
    if (endAngle < 0) endAngle += 360;

    // Одинаковые углы = полный круг
    if (startAngle == endAngle){
        circle(xc, yc, r, col);
        return;
    }

    constexpr float DEG = 0.01745329251994329577f;

    float a0 = startAngle * DEG;
    float a1 = endAngle * DEG;

    int x0 = xc + (int)roundf(cosf(a0) * r);
    int y0 = yc + (int)roundf(sinf(a0) * r);

    int x1 = xc + (int)roundf(cosf(a1) * r);
    int y1 = yc + (int)roundf(sinf(a1) * r);

    // Дуга
    arc(xc, yc, r, startAngle, endAngle, col);

    // Две стороны сектора
    line(xc, yc, x0, y0, col);
    line(xc, yc, x1, y1, col);
}

void VGA_esp32::fillPie(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col){
    auto& s = _s;
    if (!s.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    startAngle %= 360;
    endAngle %= 360;

    if (startAngle < 0) startAngle += 360;
    if (endAngle < 0) endAngle += 360;

    if (startAngle == endAngle){
        fillCircle(xc, yc, r, col);
        return;
    }

    int sweep = endAngle - startAngle;
    if (sweep < 0) sweep += 360;

    constexpr float DEG = 0.01745329251994329577f;
    constexpr int SCALE = 32767;

    // -------------------------------------------------------
    // Сначала строим ширину круга для каждой строки.
    // Используем тот же midpoint/Bresenham, что fillCircle().
    // -------------------------------------------------------

    int diameter = (r << 1) + 1;

    // r=200 -> всего 401 int = ~1.6 KB
    int *span = (int*)alloca(diameter * sizeof(int));

    for (int i = 0; i < diameter; i++)
        span[i] = -1;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    while (y >= x){

        // строки yc +/- x имеют ширину +/- y
        span[r + x] = std::max(span[r + x], y);
        span[r - x] = std::max(span[r - x], y);

        // строки yc +/- y имеют ширину +/- x
        span[r + y] = std::max(span[r + y], x);
        span[r - y] = std::max(span[r - y], x);

        x++;

        if (d > 0){
            y--;
            d += ((x - y) << 2) + 10;
        }else{
            d += (x << 2) + 6;
        }
    }

    // -------------------------------------------------------
    // Заполнение сектора <= 180°
    // -------------------------------------------------------

    auto fillSector = [&](int a0, int a1){

        float r0 = a0 * DEG;
        float r1 = a1 * DEG;

        int32_t sx = (int32_t)roundf(cosf(r0) * SCALE);
        int32_t sy = (int32_t)roundf(sinf(r0) * SCALE);

        int32_t ex = (int32_t)roundf(cosf(r1) * SCALE);
        int32_t ey = (int32_t)roundf(sinf(r1) * SCALE);

        int ya = std::max(yc - r, s.y0);
        int yb = std::min(yc + r, s.y1);

        for (int py = ya; py <= yb; py++){

            int dy = py - yc;

            // Берём X из midpoint-circle вместо sqrt()
            int xr = span[dy + r];

            if (xr < 0) continue;

            int left  = xc - xr;
            int right = xc + xr;

            if (left < s.x0) left = s.x0;
            if (right > s.x1) right = s.x1;

            if (left > right) continue;

            int minX = left - xc;
            int maxX = right - xc;

            // -------------------------
            // START ray
            // -------------------------

            if (sy > 0){
                int64_t num = (int64_t)sx * dy;
                int bound = (int)floor((double)num / sy);

                if (bound < maxX)
                    maxX = bound;
            }
            else if (sy < 0){
                int64_t num = (int64_t)sx * dy;
                int bound = (int)ceil((double)num / sy);

                if (bound > minX)
                    minX = bound;
            }
            else{
                if ((int64_t)sx * dy < 0)
                    continue;
            }

            // -------------------------
            // END ray
            // -------------------------

            if (ey > 0){
                int64_t num = (int64_t)dy * ex;
                int bound = (int)ceil((double)num / ey);

                if (bound > minX)
                    minX = bound;
            }
            else if (ey < 0){
                int64_t num = (int64_t)dy * ex;
                int bound = (int)floor((double)num / ey);

                if (bound < maxX)
                    maxX = bound;
            }
            else{
                if (-(int64_t)dy * ex < 0)
                    continue;
            }

            if (minX <= maxX)
                hLine(xc + minX, py, xc + maxX, col);
        }
    };

    // -------------------------------------------------------
    // Сам сектор
    // -------------------------------------------------------

    if (sweep <= 180){
        fillSector(startAngle, startAngle + sweep);
    }else{
        int middle = startAngle + 180;

        fillSector(startAngle, middle);
        fillSector(middle, startAngle + sweep);
    }
}

void VGA_esp32::polygon(int xc, int yc, int r, int sides, int angle, uint16_t col){
    auto& s = _s;
    if (!s.inited || r < 0 || sides < 3) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    constexpr float DEG = 0.01745329251994329577f;

    float a = angle * DEG;
    float step = (360.0f / sides) * DEG;

    // Начальный вектор
    float vx = cosf(a) * r;
    float vy = sinf(a) * r;

    // Матрица поворота на одну вершину
    float cs = cosf(step);
    float sn = sinf(step);

    int firstX = xc + (int)roundf(vx);
    int firstY = yc + (int)roundf(vy);

    int prevX = firstX;
    int prevY = firstY;

    for (int i = 1; i < sides; i++){
        float nx = vx * cs - vy * sn;
        float ny = vx * sn + vy * cs;

        vx = nx;
        vy = ny;

        int px = xc + (int)roundf(vx);
        int py = yc + (int)roundf(vy);

        line(prevX, prevY, px, py, col);

        prevX = px;
        prevY = py;
    }

    // Последняя вершина -> первая
    line(prevX, prevY, firstX, firstY, col);
}

void VGA_esp32::fillPolygon(int xc, int yc, int r, int sides, int angle, uint16_t col){
    auto& s = _s;
    if (!s.inited || r < 0 || sides < 3) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (xc + r < s.x0 || xc - r > s.x1 ||
        yc + r < s.y0 || yc - r > s.y1) return;

    constexpr float DEG = 0.01745329251994329577f;

    float a = angle * DEG;
    float step = (360.0f / sides) * DEG;

    float vx = cosf(a) * r;
    float vy = sinf(a) * r;

    float cs = cosf(step);
    float sn = sinf(step);

    // -----------------------------------------------------
    // Вершины
    // -----------------------------------------------------

    int* px = (int*)alloca(sides * sizeof(int));
    int* py = (int*)alloca(sides * sizeof(int));

    px[0] = xc + (int)roundf(vx);
    py[0] = yc + (int)roundf(vy);

    int minY = py[0];
    int maxY = py[0];

    for (int i = 1; i < sides; i++){
        float nx = vx * cs - vy * sn;
        float ny = vx * sn + vy * cs;

        vx = nx;
        vy = ny;

        px[i] = xc + (int)roundf(vx);
        py[i] = yc + (int)roundf(vy);

        if (py[i] < minY) minY = py[i];
        if (py[i] > maxY) maxY = py[i];
    }

    // -----------------------------------------------------
    // Ограничиваем Y clipping-областью
    // -----------------------------------------------------

    int drawMinY = std::max(minY, s.y0);
    int drawMaxY = std::min(maxY, s.y1);

    if (drawMinY > drawMaxY) return;

    int height = drawMaxY - drawMinY + 1;

    // -----------------------------------------------------
    // Левая и правая граница каждой строки
    // -----------------------------------------------------

    int* left  = (int*)alloca(height * sizeof(int));
    int* right = (int*)alloca(height * sizeof(int));

    for (int i = 0; i < height; i++){
        left[i]  = INT_MAX;
        right[i] = INT_MIN;
    }

    // -----------------------------------------------------
    // Растеризация всех рёбер
    // -----------------------------------------------------

    for (int i = 0, j = sides - 1; i < sides; j = i++){

        int x0 = px[j];
        int y0 = py[j];
        int x1 = px[i];
        int y1 = py[i];

        // Горизонтальное ребро отдельно
        if (y0 == y1){
            if (y0 >= drawMinY && y0 <= drawMaxY){
                int idx = y0 - drawMinY;

                int xl = std::min(x0, x1);
                int xr = std::max(x0, x1);

                if (xl < left[idx])  left[idx]  = xl;
                if (xr > right[idx]) right[idx] = xr;
            }

            continue;
        }

        // Всегда сверху вниз
        if (y0 > y1){
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        int ya = std::max(y0, drawMinY);
        int yb = std::min(y1, drawMaxY);

        if (ya > yb) continue;

        int dy = y1 - y0;
        int dx = x1 - x0;

        // 16.16 fixed point
        int64_t xFixed =
            ((int64_t)x0 << 16) +
            ((int64_t)dx * (ya - y0) << 16) / dy;

        int64_t stepFixed =
            ((int64_t)dx << 16) / dy;

        for (int y = ya; y <= yb; y++){
            int xx = (int)((xFixed + 0x8000) >> 16);
            int idx = y - drawMinY;

            if (xx < left[idx])  left[idx]  = xx;
            if (xx > right[idx]) right[idx] = xx;

            xFixed += stepFixed;
        }
    }

    // -----------------------------------------------------
    // Заполнение
    // -----------------------------------------------------

    for (int y = drawMinY; y <= drawMaxY; y++){
        int idx = y - drawMinY;

        int x0 = left[idx];
        int x1 = right[idx];

        if (x0 == INT_MAX || x1 == INT_MIN)
            continue;

        if (x0 < s.x0) x0 = s.x0;
        if (x1 > s.x1) x1 = s.x1;

        if (x0 <= x1)
            hLine(x0, y, x1, col);
    }
}

void VGA_esp32::floodFill(int x, int y, uint16_t col){
    auto& s = _s;

    if (!s.inited) return;
    if (x < s.x0 || x > s.x1 || y < s.y0 || y > s.y1) return;

    struct Span{
        int16_t x1;
        int16_t x2;
        int16_t y;
    };

    constexpr int STACK_SIZE = 2048;

    Span* stack = (Span*)malloc(STACK_SIZE * sizeof(Span));
    if (!stack) return;

    int sp = 0;

    // ========================================================
    // 8 BIT / PALETTE
    // ========================================================

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        uint8_t newCol = (uint8_t)col;
        uint8_t oldCol = s.bLine8[y][x];

        if (oldCol == newCol){
            free(stack);
            return;
        }

        uint8_t* row = s.bLine8[y];

        // ----------------------------------------------------
        // Первый span
        // ----------------------------------------------------

        int xl = x;
        int xr = x;

        while (xl > s.x0 && row[xl - 1] == oldCol)
            xl--;

        while (xr < s.x1 && row[xr + 1] == oldCol)
            xr++;

        memset(row + xl, newCol, xr - xl + 1);

        stack[sp++] = {
            (int16_t)xl,
            (int16_t)xr,
            (int16_t)y
        };

        // ----------------------------------------------------
        // Обработка spans
        // ----------------------------------------------------

        while (sp > 0){
            Span p = stack[--sp];

            // Строка сверху и снизу
            for (int dir = -1; dir <= 1; dir += 2){
                int ny = p.y + dir;

                if (ny < s.y0 || ny > s.y1)
                    continue;

                uint8_t* scan = s.bLine8[ny];

                int xx = p.x1;

                while (xx <= p.x2){

                    // Пропускаем пиксели другого цвета
                    while (xx <= p.x2 && scan[xx] != oldCol)
                        xx++;

                    if (xx > p.x2)
                        break;

                    // ----------------------------------------
                    // Начало нового span
                    // ----------------------------------------

                    int start = xx;

                    while (start > s.x0 && scan[start - 1] == oldCol)
                        start--;

                    // ----------------------------------------
                    // Конец нового span
                    // ----------------------------------------

                    int end = xx;

                    while (end < s.x1 && scan[end + 1] == oldCol)
                        end++;

                    // ----------------------------------------
                    // Быстрая заливка 8-bit
                    // ----------------------------------------

                    memset(
                        scan + start,
                        newCol,
                        end - start + 1
                    );

                    // ----------------------------------------
                    // Проверка переполнения стека
                    // ----------------------------------------

                    if (sp >= STACK_SIZE){
                        free(stack);
                        return;
                    }

                    stack[sp++] = {
                        (int16_t)start,
                        (int16_t)end,
                        (int16_t)ny
                    };

                    xx = end + 1;
                }
            }
        }
    }

    // ========================================================
    // RGB565 16 BIT
    // ========================================================

    else{
        uint16_t newCol = col;
        uint16_t oldCol = s.bLine16[y][x];

        if (oldCol == newCol){
            free(stack);
            return;
        }

        // ----------------------------------------------------
        // Быстрая заливка RGB565
        //
        // После выравнивания пишем два пикселя
        // одной 32-bit операцией.
        // ----------------------------------------------------

        auto fill16 = [](uint16_t* dst, int count, uint16_t color){
            if (count <= 0) return;

            // Выравниваем dst на 32-bit
            if ((uintptr_t)dst & 3){
                *dst++ = color;
                count--;
            }

            uint32_t color32 =
                (uint32_t)color |
                ((uint32_t)color << 16);

            uint32_t* dst32 = (uint32_t*)dst;

            while (count >= 2){
                *dst32++ = color32;
                count -= 2;
            }

            // Нечётный последний пиксель
            if (count)
                *(uint16_t*)dst32 = color;
        };

        uint16_t* row = s.bLine16[y];

        // ----------------------------------------------------
        // Первый span
        // ----------------------------------------------------

        int xl = x;
        int xr = x;

        while (xl > s.x0 && row[xl - 1] == oldCol)
            xl--;

        while (xr < s.x1 && row[xr + 1] == oldCol)
            xr++;

        fill16(
            row + xl,
            xr - xl + 1,
            newCol
        );

        stack[sp++] = {
            (int16_t)xl,
            (int16_t)xr,
            (int16_t)y
        };

        // ----------------------------------------------------
        // Обработка spans
        // ----------------------------------------------------

        while (sp > 0){
            Span p = stack[--sp];

            // Строка сверху и снизу
            for (int dir = -1; dir <= 1; dir += 2){
                int ny = p.y + dir;

                if (ny < s.y0 || ny > s.y1)
                    continue;

                uint16_t* scan = s.bLine16[ny];

                int xx = p.x1;

                while (xx <= p.x2){

                    // Пропускаем пиксели другого цвета
                    while (xx <= p.x2 && scan[xx] != oldCol)
                        xx++;

                    if (xx > p.x2)
                        break;

                    // ----------------------------------------
                    // Начало span
                    // ----------------------------------------

                    int start = xx;

                    while (start > s.x0 && scan[start - 1] == oldCol)
                        start--;

                    // ----------------------------------------
                    // Конец span
                    // ----------------------------------------

                    int end = xx;

                    while (end < s.x1 && scan[end + 1] == oldCol)
                        end++;

                    // ----------------------------------------
                    // Быстрая RGB565 заливка
                    // ----------------------------------------

                    fill16(
                        scan + start,
                        end - start + 1,
                        newCol
                    );

                    // ----------------------------------------
                    // Проверка переполнения стека
                    // ----------------------------------------

                    if (sp >= STACK_SIZE){
                        free(stack);
                        return;
                    }

                    stack[sp++] = {
                        (int16_t)start,
                        (int16_t)end,
                        (int16_t)ny
                    };

                    xx = end + 1;
                }
            }
        }
    }

    free(stack);
}

void VGA_esp32::star(int x, int y, int radius, int sides, int rotation, uint16_t col){
    auto& s = _s;

    if (!s.inited || radius < 0 || sides < 2) return;

    if (radius == 0){
        putPixel(x, y, col);
        return;
    }

    // Полностью вне clipping area
    if (x + radius < s.x0 || x - radius > s.x1 ||
        y + radius < s.y0 || y - radius > s.y1)
        return;

    constexpr float DEG = 0.01745329251994329577f;

    // Внутренний радиус = 45% внешнего
    float innerScale = 0.45f;

    // Всего вершин в 2 раза больше числа лучей
    int vertices = sides << 1;

    // Угол между соседними вершинами
    float step = (180.0f / sides) * DEG;
    float angle = rotation * DEG;

    // ========================================================
    // Начальный единичный вектор
    // ========================================================

    float vx = cosf(angle);
    float vy = sinf(angle);

    // ========================================================
    // Матрица поворота на один шаг
    //
    // [ cos -sin ]
    // [ sin  cos ]
    // ========================================================

    float cs = cosf(step);
    float sn = sinf(step);

    // ========================================================
    // Первая вершина всегда внешняя
    // ========================================================

    int firstX = x + (int)roundf(vx * radius);
    int firstY = y + (int)roundf(vy * radius);

    int prevX = firstX;
    int prevY = firstY;

    // ========================================================
    // Остальные вершины
    // ========================================================

    for (int i = 1; i < vertices; i++){

        // Поворачиваем единичный вектор
        float nx = vx * cs - vy * sn;
        float ny = vx * sn + vy * cs;

        vx = nx;
        vy = ny;

        float r;

        if (i & 1)
            r = radius * innerScale;
        else
            r = radius;

        int px = x + (int)roundf(vx * r);
        int py = y + (int)roundf(vy * r);

        line(
            prevX,
            prevY,
            px,
            py,
            col
        );

        prevX = px;
        prevY = py;
    }

    // ========================================================
    // Замыкаем последнюю вершину с первой
    // ========================================================

    line(
        prevX,
        prevY,
        firstX,
        firstY,
        col
    );
}

void VGA_esp32::fillStar(int xc, int yc, int radius, int sides, int rotation, uint16_t col){
    auto& s = _s;

    if (!s.inited || radius < 0 || sides < 2) return;

    constexpr int MAX_SIDES = 64;
    constexpr int MAX_EDGES = MAX_SIDES * 2;

    if (sides > MAX_SIDES) return;

    if (radius == 0){
        putPixel(xc, yc, col);
        return;
    }

    if (xc + radius < s.x0 || xc - radius > s.x1 ||
        yc + radius < s.y0 || yc - radius > s.y1)
        return;

    constexpr float DEG = 0.01745329251994329577f;

    struct Edge{
        int16_t yStart;
        int16_t yEnd;
        int32_t x;
        int32_t dx;
    };

    int16_t px[MAX_EDGES];
    int16_t py[MAX_EDGES];

    Edge edges[MAX_EDGES];
    Edge active[MAX_EDGES];

    int16_t nodes[MAX_EDGES];

    int count = sides << 1;

    // ========================================================
    // Генерация вершин
    // ========================================================

    float innerRadius = radius * 0.45f;

    float angle = rotation * DEG;
    float step = (180.0f / sides) * DEG;

    float vx = cosf(angle);
    float vy = sinf(angle);

    float cs = cosf(step);
    float sn = sinf(step);

    int minY = INT_MAX;
    int maxY = INT_MIN;

    for (int i = 0; i < count; i++){
        float r = (i & 1) ? innerRadius : (float)radius;

        int xx = xc + (int)roundf(vx * r);
        int yy = yc + (int)roundf(vy * r);

        px[i] = (int16_t)xx;
        py[i] = (int16_t)yy;

        if (yy < minY) minY = yy;
        if (yy > maxY) maxY = yy;

        float nx = vx * cs - vy * sn;
        float ny = vx * sn + vy * cs;

        vx = nx;
        vy = ny;
    }

    // ========================================================
    // Y clipping
    // ========================================================

    int drawMinY = std::max(minY, s.y0);
    int drawMaxY = std::min(maxY, s.y1);

    if (drawMinY > drawMaxY) return;

    // ========================================================
    // Формируем Edge Table
    // ========================================================

    int edgeCount = 0;

    for (int i = 0, j = count - 1; i < count; j = i++){
        int x0 = px[j];
        int y0 = py[j];

        int x1 = px[i];
        int y1 = py[i];

        // Горизонтальное ребро не участвует в even-odd
        if (y0 == y1) continue;

        // Всегда сверху вниз
        if (y0 > y1){
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        int dy = y1 - y0;
        int dx = x1 - x0;

        // Полуоткрытый диапазон:
        // y0 < y <= y1

        int yStart = y0 + 1;
        int yEnd = y1;

        if (yStart < drawMinY) yStart = drawMinY;
        if (yEnd > drawMaxY) yEnd = drawMaxY;

        if (yStart > yEnd) continue;

        Edge& e = edges[edgeCount++];

        e.yStart = (int16_t)yStart;
        e.yEnd = (int16_t)yEnd;

        // X increment для каждой следующей строки, 16.16
        e.dx = (int32_t)(((int64_t)dx << 16) / dy);

        // Начальный X на yStart
        e.x = (int32_t)(
            ((int64_t)x0 << 16) +
            (int64_t)e.dx * (yStart - y0)
        );
    }

    // ========================================================
    // Сортируем рёбра по yStart
    //
    // Это выполняется только один раз.
    // ========================================================

    for (int i = 1; i < edgeCount; i++){
        Edge value = edges[i];
        int j = i - 1;

        while (j >= 0 && edges[j].yStart > value.yStart){
            edges[j + 1] = edges[j];
            j--;
        }

        edges[j + 1] = value;
    }

    // ========================================================
    // Active Edge Table
    // ========================================================

    int activeCount = 0;
    int nextEdge = 0;

    for (int y = drawMinY; y <= drawMaxY; y++){

        // ----------------------------------------------------
        // Удаляем закончившиеся рёбра
        // ----------------------------------------------------

        int dst = 0;

        for (int i = 0; i < activeCount; i++){
            if (active[i].yEnd >= y)
                active[dst++] = active[i];
        }

        activeCount = dst;

        // ----------------------------------------------------
        // Добавляем рёбра, которые начинаются на этой строке
        //
        // edges[] уже отсортирован по yStart.
        // Поэтому полного перебора edgeCount больше нет.
        // ----------------------------------------------------

        while (nextEdge < edgeCount &&
               edges[nextEdge].yStart == y){

            active[activeCount++] = edges[nextEdge++];
        }

        // ----------------------------------------------------
        // Получаем X пересечения
        // ----------------------------------------------------

        int nodeCount = activeCount;

        for (int i = 0; i < nodeCount; i++)
            nodes[i] = (int16_t)(active[i].x >> 16);

        // ----------------------------------------------------
        // Сортируем X
        // ----------------------------------------------------

        for (int i = 1; i < nodeCount; i++){
            int16_t value = nodes[i];
            int j = i - 1;

            while (j >= 0 && nodes[j] > value){
                nodes[j + 1] = nodes[j];
                j--;
            }

            nodes[j + 1] = value;
        }

        // ----------------------------------------------------
        // Even-Odd fill
        // ----------------------------------------------------

        for (int i = 0; i + 1 < nodeCount; i += 2){
            int x0 = nodes[i];
            int x1 = nodes[i + 1];

            if (x0 > x1)
                std::swap(x0, x1);

            if (x0 < s.x0) x0 = s.x0;
            if (x1 > s.x1) x1 = s.x1;

            if (x0 <= x1)
                hLine(x0, y, x1, col);
        }

        // ----------------------------------------------------
        // Переходим на следующую строку
        // ----------------------------------------------------

        for (int i = 0; i < activeCount; i++)
            active[i].x += active[i].dx;
    }
}

void VGA_esp32::blur() {
    auto& s = _s;
    if (!s.inited) return;

    int width  = _s.width;
    int height = _s.height;

    if (s.usePal || s.bpp == ColorDepth::BPP_8){
        uint8_t* buf = s.bLine8[0];

        for (int y = 0; y < height - 1; y++) {
            uint8_t* line0 = buf + y * width;
            uint8_t* line1 = buf + (y + 1) * width;

            for (int x = 1; x < width - 1; x++) {
                uint8_t c0 = line0[x];
                uint8_t c1 = line1[x];
                uint8_t c2 = line1[x - 1];
                uint8_t c3 = line1[x + 1];

                int r = (R8(c0) + R8(c1) + ((R8(c2) + R8(c3)) >> 1)) / 3;
                int g = (G8(c0) + G8(c1) + ((G8(c2) + G8(c3)) >> 1)) / 3;
                int b = (B8(c0) + B8(c1) + ((B8(c2) + B8(c3)) >> 1)) / 3;

                line0[x] = (r << 5) | (g << 2) | b;
            }
        }
    } else {
        uint16_t* buf = s.bLine16[0];

        for (int y = 0; y < height - 1; y++) {
            uint16_t* line0 = buf + y * width;
            uint16_t* line1 = buf + (y + 1) * width;

            for (int x = 1; x < width - 1; x++) {
                uint16_t c0 = line0[x];
                uint16_t c1 = line1[x];
                uint16_t c2 = line1[x - 1];
                uint16_t c3 = line1[x + 1];

                int r = (R16(c0) + R16(c1) + ((R16(c2) + R16(c3)) >> 1)) / 3;
                int g = (G16(c0) + G16(c1) + ((G16(c2) + G16(c3)) >> 1)) / 3;
                int b = (B16(c0) + B16(c1) + ((B16(c2) + B16(c3)) >> 1)) / 3;

                line0[x] = (r << 11) | (g << 5) | b;
            }
        }
    }
}