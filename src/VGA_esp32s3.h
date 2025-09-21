#pragma once

#include "esp_async_memcpy.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <esp_LCD_panel_rgb.h>

#define PTR_OFFSET(ptr, offset)         ((void*)((uint8_t*)(ptr) + (offset)))
#define PTR_OFFSET_T(ptr, offset, type) ((type*)((uint8_t*)(ptr) + (offset)))

//VGA connect pins ----------------------------------------------------------------------------------------------------
//R
#define VGA_PIN_NUM_DATA0          4
#define VGA_PIN_NUM_DATA1          5
#define VGA_PIN_NUM_DATA2          6
#define VGA_PIN_NUM_DATA3          7
#define VGA_PIN_NUM_DATA4          15

//G
#define VGA_PIN_NUM_DATA5          16
#define VGA_PIN_NUM_DATA6          17
#define VGA_PIN_NUM_DATA7          18
#define VGA_PIN_NUM_DATA8          8
#define VGA_PIN_NUM_DATA9          9
#define VGA_PIN_NUM_DATA10         14

//B
#define VGA_PIN_NUM_DATA11         10
#define VGA_PIN_NUM_DATA12         11
#define VGA_PIN_NUM_DATA13         12
#define VGA_PIN_NUM_DATA14         13
#define VGA_PIN_NUM_DATA15         21

//Other
#define VGA_PIN_NUM_DISP_EN     -1
#define VGA_PIN_NUM_PCLK        -1
#define VGA_PIN_NUM_DE          -1
#define VGA_PIN_NUM_VSYNC       2
#define VGA_PIN_NUM_HSYNC       1

class VGA_esp32s3{
    public:
        //Pixel clock, hRes, VRes, ColBit, BPP
        //H front porch, H pulse width, H back porch, 
        //V front porch, V pulse width, V back porch, 
        //Scale, _bounce_buffer_size_px

        //Mode ok
        static constexpr int MODE512x384x8[] = {20'000'000, 512, 384, 8, 8, 8, 62, 42, 1, 5, 26, 16384};//+++
        static constexpr int MODE640x350x8[] = {25'000'000, 640, 350, 8, 8, 16, 96, 48, 37, 2, 60, 16000};//+--
        static constexpr int MODE640x400x8[] = {25'000'000, 640, 400, 8, 8, 16, 96, 48, 12, 2, 35, 25600};//+++
        static constexpr int MODE640x480x8[] = {25'000'000, 640, 480, 8, 8, 16, 96, 48, 10, 2, 33, 30720};//+++
        static constexpr int MODE720x400x8[] = {30'000'000, 720, 400, 8, 8, 36, 72, 108, 1, 3, 42, 14400};//+++
        static constexpr int MODE768x576x8[] = {30'000'000, 768, 576, 8, 8, 24, 80, 104, 1, 3, 17, 18432};//+++
        static constexpr int MODE800x600x8[] = {40'000'000, 800, 600, 8, 8, 40, 128, 88, 1, 4, 23, 16000};//+++
        static constexpr int MODE1024x768x8[] = {65'000'000, 1024, 768, 8, 8, 8, 176, 56, 0, 8, 41, 49152};//-++   

        static constexpr int MODE512x384x16[] = {20'000'000, 512, 384, 16, 16, 8, 62, 42, 1, 5, 26, 16384};//+++
        static constexpr int MODE640x350x16[] = {25'000'000, 640, 350, 16, 16, 16, 96, 48, 37, 2, 60, 16000};//+--
        static constexpr int MODE640x400x16[] = {25'000'000, 640, 400, 16, 16, 16, 96, 48, 12, 2, 35, 25600};//+++
        static constexpr int MODE640x480x16[] = {25'000'000, 640, 480, 16, 16, 16, 96, 48, 10, 2, 33, 15360};//-++
        static constexpr int MODE720x400x16[] = {30'000'000, 720, 400, 16, 16, 36, 72, 108, 1, 3, 42, 14400};//+++
        static constexpr int MODE768x576x16[] = {30'000'000, 768, 576, 16, 16, 24, 80, 104, 1, 3, 17, 18432};//-++
        static constexpr int MODE800x600x16[] = {40'000'000, 800, 600, 16, 16, 40, 128, 88, 1, 4, 23, 16000};//---
        static constexpr int MODE1024x768x16[] = {65'000'000, 1024, 768, 16, 16, 8, 176, 56, 0, 8, 41, 49152};//---  

        //               r  r  r  r  r   g   g   g   g  g  g   b   b   b   b   b   h  v
        //               0  1  2  3  4   5   6   7   8  9  10  11  12  13  14  15  16 17
        int _pins[18] = {4, 5, 6, 7, 15, 16, 17, 18, 8, 9, 14, 10, 11, 12, 13, 21, 1, 2};

        uint8_t*    _buf8 = nullptr;
		uint16_t*   _buf16 = nullptr; 
        int*        _fastY = nullptr;
        uint8_t*    _tmpBuf = nullptr;
        int         _frontBuff, _backBuff;

        VGA_esp32s3();         // Конструктор 
        ~VGA_esp32s3();        // Деструктор

        int Width()         {return _width;};
        int Height()        {return _height;};
        int ColBit()        {return _colBit;};
        int BPP()           {return _bpp;};
        int BPPShift()      {return _bppShift;};
        int Scale()         {return _scale;};

        int Aspect()        {return _aspect;};
        int ScrWidth()      {return _scrWidth;};
        int ScrHeight()     {return _scrHeight;};
        int ScrCX()         {return _scrCX;};
        int ScrCY()         {return _scrCY;};               
        int ScrXX()         {return _scrXX;};
        int ScrYY()         {return _scrYY;};
        int ScrSize()       {return _scrSize;};
        int ScrFullSize()   {return _scrFullSize;};

        int MaxCol()        {return 1 << _bpp;};

        //Viewport
        int vX1()           {return _vX1;};
        int vX2()           {return _vX2;};
        int vY1()           {return _vY1;};
        int vY2()           {return _vY2;};
        int vWidth()        {return _vWidth;};
        int vHeight()       {return _vHeight;};
        int vCX()           {return _vCX;};
        int vCY()           {return _vCY;};
        int vXX()           {return _vXX;};
        int vYY()           {return _vYY;};

        void setPins(uint8_t r0, uint8_t r1, uint8_t r2, uint8_t r3, uint8_t r4, 
                     uint8_t g0, uint8_t g1, uint8_t g2, uint8_t g3, uint8_t g4, uint8_t g5,
                     uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                     uint8_t h_sync = 1, uint8_t v_sync = 2);

        bool init(const int *mode, int scale = 0, bool dBuff = true, bool psRam = true);
        void printInfo();
        void setViewport(int x1, int y1, int x2, int y2);
        
        //Screen scroll
        void scrollX(int sx);
        void scrollY(int sy);
        void scrollXY(int sx, int sy);

        //Window sxroll
        void winScrollX(int x1, int y1, int x2, int y2, int sx);
        void winScrollY(int x1, int y1, int x2, int y2, int sy);
        void winScrollXY(int x1, int y1, int x2, int y2, int sx, int sy);

        //Copy screen
        void copyScr(bool backToFront = true);
        void copyScrRect(int x, int y, int x1, int y1, int x2, int y2, bool backToFront = true);
        void blit(int dx, int dy, int sx1, int sy1, int sx2, int sy2, bool backToFront);
        
        void swap();   

   protected:
        //Параметры режима
        int _pclk_hz;

        int _hsync_back_porch;
        int _hsync_front_porch;
        int _hsync_pulse_width;
        int _vsync_back_porch;
        int _vsync_front_porch;
        int _vsync_pulse_width; 
        int _scale; 
        size_t _bounce_buffer_size_px, _lastBounceBufferPos;
        bool _psRam, _dBuff;

        int _lines, _tik;
        int _width2X, _width4X;
        int _scrWidth2X, _scrWidth4X;
        int _tmpBufSize;

        //Mode
        int _bppShift;
        int _width, _height;
        int _size;
        int _colBit, _bpp;
        
        //Screen
        float _aspect;
        int _scrWidth, _scrHeight;
        int _scrCX, _scrCY;
        int _scrXX, _scrYY;
        int _scrSize, _scrFullSize;

        //Viewport
        int _vX1, _vX2;
        int _vY1, _vY2;
        int _vWidth, _vHeight;        
        int _vCX, _vCY;
        int _vXX, _vYY;

        esp_lcd_panel_handle_t _panel_handle = NULL;
        static bool IRAM_ATTR on_bounce_empty           (esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
        static bool IRAM_ATTR on_color_trans_done       (esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_vsync                  (esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_bounce_frame_finish    (esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_frame_buf_complete     (esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);

	    SemaphoreHandle_t _sem_vsync_end;
	    SemaphoreHandle_t _sem_gui_ready;

        bool setPanelConfig();
        void regCallbackSemaphore();  
};    

