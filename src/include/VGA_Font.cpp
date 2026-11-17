#include "../VGA_esp32.h"
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>

bool VGA_esp32::destroyFont(){
    auto& f = _fnt;
    if (f.buf)
    free(f.buf);
    f.buf = nullptr;
    f.created = false;
    return false;
}
bool VGA_esp32::loadFont(const uint8_t* data){
    destroyFont();
    if (!data)
    return false;
    auto& f = _fnt;
    f.width  = *data++;
    f.height = *data++;
    if (f.width <= 1 || f.height <= 1)
    return destroyFont();
    f.maxX = f.width  - 1;
    f.maxY = f.height - 1;
    f.cx = f.width  >> 1;
    f.cy = f.height >> 1;
    const size_t bytesPerRow =
    (f.width + 7u) >> 3;
    const size_t bytesPerChar =
    bytesPerRow * f.height;
    f.size =
    bytesPerChar * 256u;
    if (!(f.buf = allocateMemory(f.size, false)))
    return destroyFont();
    memcpy( f.buf, data, f.size );
    f.created = true;
    return true;
}
void VGA_esp32::putChar( int x, int y, char ch, uint16_t col ){
    auto& s = _s;
    auto& f = _fnt;
    if (!s.inited || !f.created || !f.buf)
    return;
    int xx = x + f.maxX;
    int yy = y + f.maxY;
    if ( x  > s.x1 || y  > s.y1 || xx < s.x0 || yy < s.y0 ){
        return;
    }
    const int bytesPerRow =
    (f.width + 7) >> 3;
    const int bytesPerChar =
    bytesPerRow * f.height;
    const uint8_t* c =
    (const uint8_t*)f.buf +
    (uint8_t)ch * bytesPerChar;
    if ( f.width <= 8 && x  >= s.x0 && y  >= s.y0 && xx <= s.x1 && yy <= s.y1 ){
        int lines = f.height;
        if (s.usePal || s.bpp == BPP_8){
            uint8_t color =
            (uint8_t)col;
            uint8_t* scr =
            &s.bLine8[y][x];
            const int skip =
            s.width - f.width;
            while (lines--){
                uint8_t bits =
                *c++;
                int px =
                f.width;
                while (px--){
                    if (bits & 0x80)
                    *scr = color;
                    bits <<= 1;
                    scr++;
                }
                scr += skip;
            }
        } else {
            uint16_t* scr =
            &s.bLine16[y][x];
            const int skip =
            s.width - f.width;
            while (lines--){
                uint8_t bits =
                *c++;
                int px =
                f.width;
                while (px--){
                    if (bits & 0x80)
                    *scr = col;
                    bits <<= 1;
                    scr++;
                }
                scr += skip;
            }
        }
        return;
    }
    if ( x  >= s.x0 && y  >= s.y0 && xx <= s.x1 && yy <= s.y1 ){
        if (s.usePal || s.bpp == BPP_8){
            uint8_t color =
            (uint8_t)col;
            for ( int py = 0; py < f.height; py++ ){
                uint8_t* scr =
                &s.bLine8[y + py][x];
                const uint8_t* row =
                c + py * bytesPerRow;
                int px = 0;
                for ( int b = 0; b < bytesPerRow; b++ ){
                    uint8_t bits =
                    row[b];
                    int count =
                    std::min( 8, f.width - px );
                    while (count--){
                        if (bits & 0x80)
                        *scr = color;
                        bits <<= 1;
                        scr++;
                        px++;
                    }
                }
            }
        } else {
            for ( int py = 0; py < f.height; py++ ){
                uint16_t* scr =
                &s.bLine16[y + py][x];
                const uint8_t* row =
                c + py * bytesPerRow;
                int px = 0;
                for ( int b = 0; b < bytesPerRow; b++ ){
                    uint8_t bits =
                    row[b];
                    int count =
                    std::min( 8, f.width - px );
                    while (count--){
                        if (bits & 0x80)
                        *scr = col;
                        bits <<= 1;
                        scr++;
                        px++;
                    }
                }
            }
        }
        return;
    }
    int sxl =
    (x < s.x0 ? s.x0 - x : 0);
    int sxr =
    (xx > s.x1 ? xx - s.x1 : 0);
    int syu =
    (y < s.y0 ? s.y0 - y : 0);
    int syd =
    (yy > s.y1 ? yy - s.y1 : 0);
    int copyX =
    f.width - sxl - sxr;
    int copyY =
    f.height - syu - syd;
    if (copyX <= 0 || copyY <= 0)
    return;
    int sox =
    x + sxl;
    int soy =
    y + syu;
    if (s.usePal || s.bpp == BPP_8){
        uint8_t color =
        (uint8_t)col;
        for ( int py = 0; py < copyY; py++ ){
            uint8_t* scr =
            &s.bLine8[soy + py][sox];
            const uint8_t* row =
            c +
            (syu + py) *
            bytesPerRow;
            for ( int px = 0; px < copyX; px++ ){
                int fx =
                sxl + px;
                if ( row[fx >> 3] & (0x80 >> (fx & 7)) ){
                    scr[px] = color;
                }
            }
        }
    } else {
        for ( int py = 0; py < copyY; py++ ){
            uint16_t* scr =
            &s.bLine16[soy + py][sox];
            const uint8_t* row =
            c +
            (syu + py) *
            bytesPerRow;
            for ( int px = 0; px < copyX; px++ ){
                int fx =
                sxl + px;
                if ( row[fx >> 3] & (0x80 >> (fx & 7)) ){
                    scr[px] = col;
                }
            }
        }
    }
}
void VGA_esp32::putCharScaled( int x, int y, char ch, uint16_t col ){
    auto& s = _s;
    auto& f = _fnt;
    if (!s.inited || !f.created || !f.buf)
    return;
    const int scale =
    f.textSize;
    if (scale <= 1){
        putChar( x, y, ch, col );
        return;
    }
    const int charW =
    f.width * scale;
    const int charH =
    f.height * scale;
    const int xx =
    x + charW - 1;
    const int yy =
    y + charH - 1;
    if ( x  > s.x1 || y  > s.y1 || xx < s.x0 || yy < s.y0 ){
        return;
    }
    const int bytesPerRow =
    (f.width + 7) >> 3;
    const int bytesPerChar =
    bytesPerRow * f.height;
    const uint8_t* c =
    (const uint8_t*)f.buf +
    (uint8_t)ch * bytesPerChar;
    if (s.usePal || s.bpp == BPP_8){
        const uint8_t color =
        (uint8_t)col;
        for ( int fy = 0; fy < f.height; fy++ ){
            const uint8_t* row =
            c + fy * bytesPerRow;
            const int sy =
            y + fy * scale;
            for ( int fx = 0; fx < f.width; fx++ ){
                if (!( row[fx >> 3] & (0x80 >> (fx & 7)) )){
                    continue;
                }
                const int sx =
                x + fx * scale;
                for ( int dyOffset = 0; dyOffset < scale; dyOffset++ ){
                    const int dy =
                    sy + dyOffset;
                    if ( dy < s.y0 || dy > s.y1 ){
                        continue;
                    }
                    uint8_t* dst =
                    s.bLine8[dy];
                    for ( int dxOffset = 0; dxOffset < scale; dxOffset++ ){
                        const int dx =
                        sx + dxOffset;
                        if ( dx >= s.x0 && dx <= s.x1 ){
                            dst[dx] = color;
                        }
                    }
                }
            }
        }
    } else {
        for ( int fy = 0; fy < f.height; fy++ ){
            const uint8_t* row =
            c + fy * bytesPerRow;
            const int sy =
            y + fy * scale;
            for ( int fx = 0; fx < f.width; fx++ ){
                if (!( row[fx >> 3] & (0x80 >> (fx & 7)) )){
                    continue;
                }
                const int sx =
                x + fx * scale;
                for ( int dyOffset = 0; dyOffset < scale; dyOffset++ ){
                    const int dy =
                    sy + dyOffset;
                    if ( dy < s.y0 || dy > s.y1 ){
                        continue;
                    }
                    uint16_t* dst =
                    s.bLine16[dy];
                    for ( int dxOffset = 0; dxOffset < scale; dxOffset++ ){
                        const int dx =
                        sx + dxOffset;
                        if ( dx >= s.x0 && dx <= s.x1 ){
                            dst[dx] = col;
                        }
                    }
                }
            }
        }
    }
}
void VGA_esp32::drawChar( int x, int y, char ch, uint16_t col ){
    if (_fnt.textSize <= 1){
        putChar( x, y, ch, col );
    } else {
        putCharScaled( x, y, ch, col );
    }
}
void VGA_esp32::putString( int x, int y, const char* str, uint16_t col ){
    if (!str || !_fnt.created)
    return;
    auto& f = _fnt;
    auto& s = _s;
    const int charW =
    f.width * f.textSize;
    const int charH =
    f.height * f.textSize;
    int px = x;
    int py = y;
    while (*str){
        char ch =
        *str++;
        if (ch == '\n'){
            px = x;
            py += charH;
            if (py > s.y1)
            break;
            continue;
        }
        if (ch == '\r')
        continue;
        drawChar( px, py, ch, col );
        px += charW;
        if ( px + charW - 1 > s.x1 ){
            px = x;
            py += charH;
            if (py > s.y1)
            break;
        }
    }
}
void VGA_esp32::putInt( int x, int y, int value, uint16_t col ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%d", value );
    putString( x, y, buf, col );
}
void VGA_esp32::putUInt( int x, int y, unsigned int value, uint16_t col ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%u", value );
    putString( x, y, buf, col );
}
void VGA_esp32::putLong( int x, int y, long value, uint16_t col ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%ld", value );
    putString( x, y, buf, col );
}
void VGA_esp32::putULong( int x, int y, unsigned long value, uint16_t col ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%lu", value );
    putString( x, y, buf, col );
}
void VGA_esp32::putPrintf( int x, int y, uint16_t col, const char* format, ... ){
    if (!format)
    return;
    char buf[256];
    va_list args;
    va_start( args, format );
    vsnprintf( buf, sizeof(buf), format, args );
    va_end(args);
    putString( x, y, buf, col );
}
void VGA_esp32::setCursor( int x, int y ){
    auto& f = _fnt;
    f.cursorX = x;
    f.cursorY = y;
    f.cursorStartX = x;
}
void VGA_esp32::setTextColor( uint16_t col ){
    _fnt.textColor = col;
}
void VGA_esp32::setTextSize( uint8_t size ){
    if (size < 1)
    size = 1;
    if (size > 8)
    size = 8;
    _fnt.textSize = size;
}
void VGA_esp32::print(char ch){
    auto& f = _fnt;
    auto& s = _s;
    if (!f.created)
    return;
    const int charW =
    f.width * f.textSize;
    const int charH =
    f.height * f.textSize;
    if (ch == '\n'){
        f.cursorX =
        f.cursorStartX;
        f.cursorY +=
        charH;
        return;
    }
    if (ch == '\r')
    return;
    drawChar( f.cursorX, f.cursorY, ch, f.textColor );
    f.cursorX +=
    charW;
    if ( f.cursorX + charW - 1 > s.x1 ){
        f.cursorX =
        f.cursorStartX;
        f.cursorY +=
        charH;
    }
}
void VGA_esp32::print( const char* str ){
    if (!str)
    return;
    while (*str)
    print(*str++);
}
void VGA_esp32::print( int value ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%d", value );
    print(buf);
}
void VGA_esp32::print( unsigned int value ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%u", value );
    print(buf);
}
void VGA_esp32::print( long value ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%ld", value );
    print(buf);
}
void VGA_esp32::print( unsigned long value ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%lu", value );
    print(buf);
}
void VGA_esp32::printf( const char* format, ... ){
    if (!format)
    return;
    char buf[256];
    va_list args;
    va_start( args, format );
    vsnprintf( buf, sizeof(buf), format, args );
    va_end(args);
    print(buf);
}
void VGA_esp32::println(){
    print('\n');
}
void VGA_esp32::println( const char* str ){
    print(str);
    print('\n');
}
void VGA_esp32::println( char ch ){
    print(ch);
    print('\n');
}
void VGA_esp32::println( int value ){
    print(value);
    print('\n');
}
void VGA_esp32::println( unsigned int value ){
    print(value);
    print('\n');
}
void VGA_esp32::println( long value ){
    print(value);
    print('\n');
}
void VGA_esp32::println( unsigned long value ){
    print(value);
    print('\n');
}
void VGA_esp32::drawAffineChar( float dstX, float dstY, char ch, uint16_t col, float rad, float zoomX, float zoomY ){
    auto& s = _s;
    auto& f = _fnt;
    if ( !s.inited || !f.created || !f.buf ){
        return;
    }
    const int fontW =
    f.width;
    const int fontH =
    f.height;
    const int bytesPerRow =
    (fontW + 7) >> 3;
    const int bytesPerChar =
    bytesPerRow * fontH;
    const uint8_t* glyph =
    (const uint8_t*)f.buf +
    (uint8_t)ch * bytesPerChar;
    Affine2D m = Matrix::make( dstX, dstY, (float)f.cx, (float)f.cy, rad, zoomX, zoomY );
    RectBounds rb = Matrix::bounds( m, (float)fontW, (float)fontH );
    int x0 =
    (int)floorf(rb.sx);
    int y0 =
    (int)floorf(rb.sy);
    int x1 =
    (int)ceilf(rb.ex);
    int y1 =
    (int)ceilf(rb.ey);
    if (x0 < s.x0)
    x0 = s.x0;
    if (y0 < s.y0)
    y0 = s.y0;
    if (x1 > s.x1 + 1)
    x1 = s.x1 + 1;
    if (y1 > s.y1 + 1)
    y1 = s.y1 + 1;
    if ( x0 >= x1 || y0 >= y1 ){
        return;
    }
    Affine2DInv inv;
    if (!Matrix::invert(inv, m))
    return;
    int32_t rowU =
    inv.a * x0 +
    inv.b * y0 +
    inv.c;
    int32_t rowV =
    inv.d * x0 +
    inv.e * y0 +
    inv.f;
    if ( s.usePal || s.bpp == BPP_8 ){
        const uint8_t color =
        (uint8_t)col;
        for ( int y = y0; y < y1; y++ ){
            uint8_t* dst =
            &s.bLine8[y][x0];
            int32_t u =
            rowU;
            int32_t v =
            rowV;
            for ( int x = x0; x < x1; x++ ){
                const int sx =
                u >> FP_SHIFT;
                const int sy =
                v >> FP_SHIFT;
                if ( (unsigned)sx < (unsigned)fontW && (unsigned)sy < (unsigned)fontH ){
                    const uint8_t* srcRow =
                    glyph +
                    sy * bytesPerRow;
                    if ( srcRow[sx >> 3] & (0x80 >> (sx & 7)) ){
                        *dst = color;
                    }
                }
                dst++;
                u += inv.a;
                v += inv.d;
            }
            rowU += inv.b;
            rowV += inv.e;
        }
    } else {
        for ( int y = y0; y < y1; y++ ){
            uint16_t* dst =
            &s.bLine16[y][x0];
            int32_t u =
            rowU;
            int32_t v =
            rowV;
            for ( int x = x0; x < x1; x++ ){
                const int sx =
                u >> FP_SHIFT;
                const int sy =
                v >> FP_SHIFT;
                if ( (unsigned)sx < (unsigned)fontW && (unsigned)sy < (unsigned)fontH ){
                    const uint8_t* srcRow =
                    glyph +
                    sy * bytesPerRow;
                    if ( srcRow[sx >> 3] & (0x80 >> (sx & 7)) ){
                        *dst = col;
                    }
                }
                dst++;
                u += inv.a;
                v += inv.d;
            }
            rowU += inv.b;
            rowV += inv.e;
        }
    }
}
void VGA_esp32::putAffineChar( int dstX, int dstY, char ch, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    auto& s = _s;
    auto& f = _fnt;
    if ( !s.inited || !f.created || !f.buf ){
        return;
    }
    if ( zoomX == 0 || zoomY == 0 ){
        return;
    }
    const float rad =
    angle(ang);
    const float zx =
    percentTo(zoomX);
    const float zy =
    percentTo(zoomY);
    drawAffineChar( (float)dstX, (float)dstY, ch, col, rad, zx, zy );
}
void VGA_esp32::putAffineString( int dstX, int dstY, const char* str, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    auto& s = _s;
    auto& f = _fnt;
    if ( !s.inited || !f.created || !f.buf || !str ){
        return;
    }
    if ( zoomX == 0 || zoomY == 0 ){
        return;
    }
    const float rad =
    angle(ang);
    const float zx =
    percentTo(zoomX);
    const float zy =
    percentTo(zoomY);
    const float cs =
    cosf(rad);
    const float sn =
    sinf(rad);
    const float advanceX =
    (float)f.width *
    zx *
    cs;
    const float advanceY =
    (float)f.width *
    zx *
    sn;
    const float lineX =
    -(float)f.height *
    zy *
    sn;
    const float lineY =
    (float)f.height *
    zy *
    cs;
    float baseX =
    (float)dstX;
    float baseY =
    (float)dstY;
    float penX =
    baseX;
    float penY =
    baseY;
    while (*str){
        const char ch =
        *str++;
        if (ch == '\n'){
            baseX += lineX;
            baseY += lineY;
            penX = baseX;
            penY = baseY;
            continue;
        }
        if (ch == '\r')
        continue;
        drawAffineChar( penX, penY, ch, col, rad, zx, zy );
        penX += advanceX;
        penY += advanceY;
    }
}
void VGA_esp32::putAffineInt( int dstX, int dstY, int value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%d", value );
    putAffineString( dstX, dstY, buf, col, ang, zoomX, zoomY );
}
void VGA_esp32::putAffineUInt( int dstX, int dstY, unsigned int value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    char buf[16];
    snprintf( buf, sizeof(buf), "%u", value );
    putAffineString( dstX, dstY, buf, col, ang, zoomX, zoomY );
}
void VGA_esp32::putAffineLong( int dstX, int dstY, long value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%ld", value );
    putAffineString( dstX, dstY, buf, col, ang, zoomX, zoomY );
}
void VGA_esp32::putAffineULong( int dstX, int dstY, unsigned long value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY ){
    char buf[24];
    snprintf( buf, sizeof(buf), "%lu", value );
    putAffineString( dstX, dstY, buf, col, ang, zoomX, zoomY );
}
void VGA_esp32::putAffinePrintf( int dstX, int dstY, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY, const char* format, ... ){
    if (!format)
    return;
    char buf[256];
    va_list args;
    va_start( args, format );
    vsnprintf( buf, sizeof(buf), format, args );
    va_end(args);
    putAffineString( dstX, dstY, buf, col, ang, zoomX, zoomY );
}
