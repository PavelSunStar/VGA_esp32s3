#pragma once

#include "VGA_esp32.h"
//#include <SpriteBank.h>

class Sprite{
    public:
        Sprite(VGA_esp32 &vga);
        ~Sprite();

        bool create(int width, int height, uint16_t images = 1, bool psRam = true);
        inline void putImage(int x, int y, uint16_t frame = 0){ image(x, y, frame, false); }
        inline void getImage(int x, int y, uint16_t frame = 0){ image(x, y, frame, true);  }
        void putSprite(int x, int y, uint16_t maskColor, uint16_t frame);
/*
        // Draw unpacked RGB332 sprite directly into VGA framebuffer.
        // maskColor < 0 disables transparency; otherwise skips RGB332 index.
        void putBankSprite(int x, int y, const BankSprite& sprite, int maskColor = -1);
        inline void putBankSprite(int x, int y, const SpriteBank& bank, uint16_t id, int maskColor = -1) {
            const BankSprite* sp = bank.get(id);
            if (sp) putBankSprite(x, y, *sp, maskColor);
        }
*/
        void putAffineSprite(int dstX, int dstY, float ang = 0, int zoomX = 100, int zoomY = 100, uint16_t maskColor = 0, uint8_t frame = 0);

    private:
        void destroy();
        int setImageDimentions(uint16_t image, int width, int height);
        bool setBufferAddr(int lines);
        void image(int x, int y, uint16_t frame = 0, bool getFrame = false);

        bool    _created  = false;
        uint16_t _images = 0;

        Image*  _img = {};

        // Buffer
        bool        _psRam  = true;
        void*       _buf    = nullptr;
        uint16_t**  _line16 = nullptr;
        uint8_t**   _line8  = nullptr;
        
        VGA_esp32 &_vga;        
};