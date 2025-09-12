#pragma once

#include "VGA_esp32s3.h"
#include "VGA_Math.h"  

class VGA_Sprite : public VGA_esp32s3 {  
    public:
        VGA_Sprite(VGA_esp32s3& vga);    
        ~VGA_Sprite(); 

        int getWidth(){return _width;}; 
        int getHeight(){return _height;};
        int getSize(){return _size;};
        int getMaxCol(){return 1 << _bpp;};
        bool isCreated(){return _created;};

        bool create(int xx, int yy, int num = 1);
        void destroy(); 

        void putImage(int x, int y, int num = 0, bool mirror_y = false);
        void putSprite(int x, int y, uint16_t maskColor = 0, int num = 0, bool mirror_x = false, bool mirror_y = false);

        void cls(uint16_t col);
        void putPixel(int x, int y, uint16_t col);
        void hLine(int x1, int y, int x2, uint16_t col);
        void vLine(int x, int y1, int y2, uint16_t col);
        void rect(int x1, int y1, int x2, int y2, uint16_t col);
        void fillRect(int x1, int y1, int x2, int y2, uint16_t col);
        void line(int x1, int y1, int x2, int y2, uint16_t col);
        void lineAngle(int x, int y, int len, int angle, uint16_t col);
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void circle(int xc, int yc, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);

    protected:
        bool allocateMemory(int xx, int yy);

        bool _created = false;
        int _width, _height, _size; 
        int _xx, _yy;
        int _scrWidth, _scrHeight; 
        int _bpp;      

        uint8_t* _img8 = nullptr;          
        uint16_t* _img16 = nullptr;       
        
        int _aniMax;
        int _aniHeight;
        size_t* _aniOffset = nullptr;

        VGA_Math _math;
        VGA_esp32s3& _vga;  
};        

