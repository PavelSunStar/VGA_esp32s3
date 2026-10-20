#pragma once

// ===============================
// PSRAM CHECK
// ===============================

#ifdef BOARD_HAS_PSRAM
    #define PSRAM_OK 1
#else
    #define PSRAM_OK 0
#endif

#define LUT_SIZE 360
#define ALIGN_UP(size, align)(((size) + ((align) - 1)) & ~((align) - 1))

enum ColorDepth : uint8_t {
    BPP_8  = 8,
    BPP_16 = 16
};

struct Pins{ 
	int r[5], g[6], b[5];
	int h, v; 
};

// ESP32 S3 =========================================================================
// ┌────────────────────────────────────────────────────────────────────────────────┐
// │ - Camera                                                                       │
// │ ~ SD card                                                                      │
// │ * PSRam                                                                        │
// ├────────────────────────────────────────────────────────────────────────────────┤
// │           _   _   _   _  _          _    _   _   _   _  _   _   _  _           │
// │ 5V,  14, 13, 12, 11, 10, 9, 46, 3,  8,  18, 17, 16, 15, 7,  6,  5, 4, EN, 3V3  │
// │ GND, 19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42, 2, 1, RX, TX   │
// │                                  *   *   *   ~   ~   ~                         │
// └────────────────────────────────────┬───────────────────────────────────────────┤
        inline Pins defPins_S3 = {  //  │                                           │
            15, 7, 6, 5, 4,         //  │   R0..R4 (5 bit)                          │
            14, 13, 12, 11, 10, 9,  //  │   G0..G5 (6 bit)                          │
            3, 8, 18, 17, 16,       //  │   B0..B5 (5 bit)                          │
            1, 2                    //  │   HSYNC, VSYNC                            │
        };                          //  │                                           │
// ┌────────────────────────────────────┴───────────────────────────────────────────┤
// │           _   _   _   _  _          _    _   _   _   _  _   _   _  _           │
// │ 5V,                         46,                                       EN, 3V3  │
// │ GND, 19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42, 2, 1, RX, TX   │
// │                                  *   *   *   ~   ~   ~                         │                                 
// └────────────────────────────────────────────────────────────────────────────────┘

struct Mode {
    int pclk_hz;
    int hRes, hFront, hSync, hBack, hPol;
    int vRes, vFront, vSync, vBack, vPol;
};

enum ScreenMode : uint8_t {
    Mode160x120,
    Mode160x200,
    Mode240x128,
    Mode256x192,
    Mode320x200,
    Mode320x240,
    Mode360x400,
    Mode400x300,
    Mode512x384,
    Mode512x480,
    Mode512x512,
    Mode528x400,
    Mode528x480,
    Mode640x200,
    Mode640x350,
    Mode640x400,
    Mode640x480,
    Mode720x348,
    Mode720x350,
    Mode720x360,
    Mode720x400,
    Mode720x480,
    Mode720x512,
    Mode720x540,
    Mode752x410,
    Mode800x560,
    Mode800x600
};

inline Mode MODE320x240_60Hz = {12587500, 320, 8, 48, 24, 0, 240, 10, 2, 33, 0};
inline Mode MODE640x480_60Hz = {25000000, 640, 16, 96, 48, 0, 480, 12, 2, 35, 0};


static const int scaleMul[] = {1, 2, 4};

struct Screen {
    bool        inited  = false;
    bool        dBuff   = false;
    bool        usePal  = false;
    uint8_t     scale   = 0;

    // Buffer
    void*       fb0     = nullptr;
    void*       fb1     = nullptr;
    uint16_t**  fLine16 = nullptr;
    uint16_t**  bLine16 = nullptr;
    uint8_t**   fLine8  = nullptr;
    uint8_t**   bLine8  = nullptr;
    uint16_t*   pal     = nullptr;
    void*       tmpBuf  = nullptr;

    // Screen config
    ColorDepth  bpp;
    uint8_t     shift    = 0;

    int         width    = 0;
    int         height   = 0;
    int         maxX     = 0;
    int         maxY     = 0;
    int         cx       = 0;
    int         cy       = 0;
    int         lineSize = 0;
    int         size     = 0;
    int         fullSize = 0;

    // Viewport
    int         x0       = 0;
    int         y0       = 0;
    int         x1       = 0;
    int         y1       = 0;
};

struct Panel{
    int lastPos = 0;
    int lines = 0;
    int pixels = 0;
    int copyBytes = 0;
    int copyBytes2x = 0;
    int copyBytes4x = 0;
    int skip = 0;
    int shift = 0;
};

struct Image{
    int width       = 0;
    int height      = 0;
    int maxX        = 0;
    int maxY        = 0;
    int cx          = 0;
    int cy          = 0;
    int lineSize    = 0;
    int size        = 0;
    int fullSize    = 0;
    int offset      = 0;
    int offsetLine  = 0;    
};

struct Font_Struct{
    bool    created = false;
    void*   buf = nullptr;
    int     width = 0;
    int     height = 0;
    int     maxX = 0;
    int     maxY = 0;
    int     cx = 0;
    int     cy = 0;
    size_t  size = 0;

    uint8_t textSize = 1;
    int cursorX = 0;
    int cursorY = 0;
    int cursorStartX = 0;
    uint16_t textColor = 0xFFFF;     
};

struct FPS_Struct {
    volatile uint32_t frame = 0;
    uint32_t frameCount = 0;
    uint32_t lastFrameCount = 0;
    uint32_t fpsStartTime = 0;
    float fps = 0.0f;
};
