#pragma once

#include "VGA_esp32s3.h"
#include "VGA_Math.h"  

class VGA_Sprite : public VGA_esp32s3 {  
    public: 
        uint8_t*    _img8 = nullptr;          
        uint16_t*   _img16 = nullptr;
        int*        _imgOffset = nullptr; 

        VGA_Sprite(VGA_esp32s3& vga);    
        ~VGA_Sprite(); 

        bool Created()  {return _created;};        
        int Width()     {return _width;};
        int Height()    {return _height;};
        int ImgOffset(int num){
            if (num < 0 || num > _maxFrame) return 0;
            return *(_imgOffset + num);
        }

        bool create         (int xx, int yy, int num = 1);
        void destroy        (); 

        void putImage       (int x, int y, int num = 0, bool mirror_y = false);
        void putSprite      (int x, int y, uint16_t maskColor = 0, int num = 0, bool mirror_x = false, bool mirror_y = false);

        void cls            (uint16_t col, int num = 0);
        void putPixel       (int x, int y, uint16_t col, int num = 0);
        void hLine          (int x1, int y, int x2, uint16_t col, int num = 0);
        void vLine          (int x, int y1, int y2, uint16_t col, int num = 0);
        void rect           (int x1, int y1, int x2, int y2, uint16_t col, int num = 0);
        void fillRect       (int x1, int y1, int x2, int y2, uint16_t col, int num = 0);
        void line           (int x1, int y1, int x2, int y2, uint16_t col, int num = 0);
        void lineAngle      (int x, int y, int len, int angle, uint16_t col, int num = 0);
        void triangle       (int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, int num = 0);
        void fillTriangle   (int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, int num = 0);
        void circle         (int xc, int yc, int r, uint16_t col, int num = 0);
        void fillCircle     (int xc, int yc, int r, uint16_t col, int num = 0);

    protected:
        bool allocateMemory(int size);
        VGA_Math _math;
        VGA_esp32s3& _vga; 

    private:        
        bool        _created = false;
        int         _width, _height, _fullHeight;
        int         _xx, _yy;
        int         _cx, _cy;

        int         _frameSize, _fullFrameSize, _fullSize; 
        int         _maxFrame;

        int         _scrWidth, _scrHeight;
        int         _bpp, _bppShift;
        int         _copyBytes, _scrSkip; 
};        

