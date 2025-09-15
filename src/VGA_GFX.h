#pragma once

#include "VGA_esp32s3.h"
#include "VGA_Math.h"  

class VGA_GFX : public VGA_esp32s3 {  
    public:
        VGA_GFX(VGA_esp32s3& vga);    
        ~VGA_GFX();    

        uint16_t getCol(uint8_t r, uint8_t g, uint8_t b);

        void cls(uint16_t col);
        void clsViewport(uint16_t col);
        void putPixel(int x, int y, uint16_t col);
        void hLine(int x1, int y, int x2, uint16_t col);
        void vLine(int x, int y1, int y2, uint16_t col);
        void rect(int x1, int y1, int x2, int y2, uint16_t col);
        void fillRect(int x1, int y1, int x2, int y2, uint16_t col);
        void line(int x1, int y1, int x2, int y2, uint16_t col);
        void lineAngle(int x, int y, int len, int angle, uint16_t col);
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void circle(int x, int y, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);
        void polygon(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void star(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col);   
        void star2(int x, int y, int radius1, int radius2, int sides, int rotation, uint16_t col);
        void fillStar2(int x, int y, int radius1, int radius2, int sides, int rotation, uint16_t col); 
        void ellipse(int xc, int yc, int rx, int ry, uint16_t col); 
        void fillEllipse(int xc, int yc, int rx, int ry, uint16_t col); 
        void circleHelper(int xc, int yc, int r, uint8_t corner, uint16_t col);
        void fillCircleHelper(int xc, int yc, int r, uint8_t corner, int ybase, uint16_t col);
        void roundRect(int x1, int y1, int x2, int y2, int r, uint16_t col); 
        void fillRoundRect(int x1, int y1, int x2, int y2, int r, uint16_t col);
        void arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col); 
        void fillArc(int xc, int yc, int r, int angle1, int angle2, uint16_t col); 
        void bezier(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col);
        void bezierCubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void spiral(int xc, int yc, int r, int turns, uint16_t col);
        void wave(int x, int y, int len, int amp, int freq, uint16_t col);

        //Vectors
        //void vPutPixel(float xn, float yn, uint16_t col);

    protected:

        
        VGA_Math _math;
        VGA_esp32s3& _vga;  // ссылка, а не копия
};    

/*
        Black        = Col16<0,   0,   0>(),       // 0
        Blue         = Col16<0,   0,   170>(),     // 1
        Green        = Col16<0,   170, 0>(),       // 2
        Cyan         = Col16<0,   170, 170>(),     // 3
        Red          = Col16<170, 0,   0>(),       // 4
        Magenta      = Col16<170, 0,   170>(),     // 5
        Brown        = Col16<170, 85,  0>(),       // 6
        LightGray    = Col16<170, 170, 170>(),     // 7
        DarkGray     = Col16<85,  85,  85>(),      // 8
        LightBlue    = Col16<85,  85,  255>(),     // 9
        LightGreen   = Col16<85,  255, 85>(),      // 10
        LightCyan    = Col16<85,  255, 255>(),     // 11
        LightRed     = Col16<255, 85,  85>(),      // 12
        LightMagenta = Col16<255, 85,  255>(),     // 13
        Yellow       = Col16<255, 255, 85>(),      // 14
        White        = Col16<255, 255, 255>()      // 15

        #pragma once

#include "VGA_esp32s3.h"  

class VGA_GFX : public VGA_esp32s3 {  
    public:
        VGA_GFX(VGA_esp32s3& vga);    
        ~VGA_GFX();    

        void initGFX();

        // Универсальный вызов
        inline void putPixel(int x, int y, uint16_t col) {
            putPixelFunc(this, x, y, col);
        }

    private:
        // Указатель на функцию для нужного BPP
        using PutPixelFunc = void (*)(VGA_GFX*, int, int, uint16_t);
        PutPixelFunc putPixelFunc;

        // Реализации для 8 и 16 бит
        static void putPixel8_static(VGA_GFX* self, int x, int y, uint16_t col);
        static void putPixel16_static(VGA_GFX* self, int x, int y, uint16_t col);

    protected:
        VGA_esp32s3& _vga;  // ссылка, а не копия
};  
*/