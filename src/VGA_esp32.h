#pragma once

#include <Arduino.h>
#include "esp_lcd_panel_rgb.h"
#include "structures.h"
#include "colorConverter.h"
#include "Matrix.h"
#include "fonts/font_def.h"
#include "VGA_Math.h"

class VGA_esp32{
    public:
        VGA_esp32();
        ~VGA_esp32();    

        //Screen 
        void* Buf()         { return _s.fb1;    }
        ColorDepth BPP()    { return _s.bpp;    }
        int Width()         { return _s.width;  }
        int Height()        { return _s.height; }
        int MaxX()          { return _s.maxX;   }
        int MaxY()          { return _s.maxY;   }
        int CX()            { return _s.cx;     }
        int CY()            { return _s.cy;     }
        int MaxCol()        { return (_s.bpp == ColorDepth::BPP_16 ? 65536 : 256); }

        float FPS()         { updateFPS(); return _f.fps; }
        uint32_t Frame()    { return _f.frame; }

        bool init(ScreenMode m = ScreenMode::Mode640x480, ColorDepth cd = ColorDepth::BPP_8, bool usePal = false, bool dBuff = false);
        void* allocateMemory(size_t size, bool psram);
        void swap();

        // Palette
        void setPalCol(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

        // Font
        bool loadFont(const uint8_t* data);

        // ------------------------------------------------------------
        // Direct drawing at exact coordinates
        // ------------------------------------------------------------

        void putChar(int x, int y, char ch, uint16_t col);
        void putString(int x, int y, const char* str, uint16_t col);
        void putInt(int x, int y, int value, uint16_t col);
        void putUInt(int x, int y, unsigned int value, uint16_t col);
        void putLong(int x, int y, long value, uint16_t col);
        void putULong(int x, int y, unsigned long value, uint16_t col);
        void putPrintf(int x, int y, uint16_t col, const char* format, ...);

        // ------------------------------------------------------------
        // Cursor text
        // ------------------------------------------------------------

        void setCursor(int x, int y);
        void setTextColor(uint16_t col);
        void setTextSize(uint8_t size);

        void print(const char* str);
        void print(char ch);
        void print(int value);
        void print(unsigned int value);
        void print(long value);
        void print(unsigned long value);

        void println();
        void println(const char* str);
        void println(char ch);
        void println(int value);
        void println(unsigned int value);
        void println(long value);
        void println(unsigned long value);
        void printf(const char* format, ...);   
        
        void putAffineChar(int dstX, int dstY, char ch, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffineString(int dstX, int dstY, const char* str, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffineInt(int dstX, int dstY, int value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffineUInt(int dstX, int dstY, unsigned int value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffineLong(int dstX, int dstY, long value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffineULong(int dstX, int dstY, unsigned long value, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putAffinePrintf(int dstX, int dstY, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY, const char* format, ...);              

        void testRGB();
        void cls(uint16_t col = 0);

        // Pixel
        uint16_t getPixel(int x, int y);
        void putPixel(int x, int y, uint16_t col);

        // Lines
        void hLine(int x0, int y, int x1, uint16_t col);
        void hLineLen(int x, int y, int len, uint16_t col);
        void vLine(int x, int y0, int y1, uint16_t col);
        void vLineLen(int x, int y, int len, uint16_t col);
        void line(int x0, int y0, int x1, int y1, uint16_t col);
        void thickLine(int x0, int y0, int x1, int y1, int width, uint16_t col);

        // Rectangles
        void rect(int x0, int y0, int x1, int y1, uint16_t col);
        void fillRect(int x0, int y0, int x1, int y1, uint16_t col);
        void roundRect(int x0, int y0, int x1, int y1, int r, uint16_t col);
        void fillRoundRect(int x0, int y0, int x1, int y1, int r, uint16_t col);

        // Triangles
        void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col);
        void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col);

        // Circles / Ellipses
        void circle(int xc, int yc, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);
        void ellipse(int xc, int yc, int rx, int ry, uint16_t col);
        void fillEllipse(int xc, int yc, int rx, int ry, uint16_t col);

        // Arcs / Pies
        void arc(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col);
        void pie(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col);
        void fillPie(int xc, int yc, int r, int startAngle, int endAngle, uint16_t col);

        // Polygons
        void polygon(int xc, int yc, int r, int sides, int angle, uint16_t col);
        void fillPolygon(int xc, int yc, int r, int sides, int angle, uint16_t col);
        void star(int x, int y, int radius, int sides, int rotation, uint16_t col);   
        void fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col);
        
        // Flood Fill
        void floodFill(int x, int y, uint16_t col);
        void blur();

    private:
        friend class Sprite;

        void updateFPS();
        void destroy();
        bool setMode(ScreenMode m);
        void setScreenDimentions();
        bool getMemory();
        void setBufferAddr();     
        
        int optimal_bounce_buffer_px();
        bool setRGBPanel();
        void regCallBack();
        bool regSemaphore();
        bool initPanel();
        void loadPalDef();

        bool destroyFont();
        void drawChar(int x, int y, char ch, uint16_t col);
        void putCharScaled(int x, int y, char ch, uint16_t col);
        void drawAffineChar(float dstX, float dstY, char ch, uint16_t col, float rad, float zoomX, float zoomY);

        static bool IRAM_ATTR on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_bounce_empty(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
        static bool IRAM_ATTR on_bounce_empty_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);

        Pins        _p  = defPins_S3;
        Mode        _m  = MODE640x480_60Hz;
        ScreenMode  _sm = Mode640x480;
        Screen      _s  = {};
        Panel       _sp = {};
        FPS_Struct  _f  = {};

        Font_Struct _fnt = {};

        esp_err_t                   err;
        esp_lcd_rgb_panel_config_t  panel_config = {};
        esp_lcd_panel_handle_t      panel_handle = nullptr; 
        
        bool                _semInited      = false;
        SemaphoreHandle_t   _sem_vsync_end  = nullptr;
        SemaphoreHandle_t   _sem_gui_ready  = nullptr; 
        
        Affine2D _mat;
        RectBounds _rectMat;  
        Affine2DInv _invMat;         
};
