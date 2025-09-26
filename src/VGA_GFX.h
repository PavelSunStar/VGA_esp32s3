#pragma once

#include "VGA_esp32s3.h"
#include "VGA_Math.h"  

// Быстрое извлечение компонент (R5 G6 B5)
// каждая уже сдвинута, чтобы её можно было складывать без доп. операций
// ---- 16 bit (RGB565) ----
inline int R16(uint16_t c) { return (c >> 11) & 0x1F; }
inline int G16(uint16_t c) { return (c >> 5)  & 0x3F; }
inline int B16(uint16_t c) { return  c        & 0x1F; }

// ---- 8 bit (RGB332) ----
inline int R8(uint8_t c) { return (c >> 5) & 0x07; }
inline int G8(uint8_t c) { return (c >> 2) & 0x07; }
inline int B8(uint8_t c) { return  c       & 0x03; }

class VGA_GFX : public VGA_esp32s3 {  
    public:
        VGA_GFX(VGA_esp32s3& vga);    
        ~VGA_GFX();    

        uint16_t getCol(uint8_t r, uint8_t g, uint8_t b);

        void cls(uint16_t col);
        void clsViewport(uint16_t col);
        void putPixel(int x, int y, uint16_t col);
        uint16_t getPixel(int x, int y);
        uint16_t getFastPixel(int x, int y);
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

        //Effect
        void blur();

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

/*
//the loop is done every frame
void loop()
{
	//setting the text cursor to the lower left corner of the screen
	videodisplay.setCursor(0, videodisplay.yres - 8);
	//setting the text color to white with opaque black background
	videodisplay.setTextColor(videodisplay.RGB(0xffffff), videodisplay.RGBA(0, 0, 0, 255));
	//printing the fps
	videodisplay.print("fps: ");
	static long f = 0;
	videodisplay.print(long((f++ * 1000) / millis()));

	//circle parameters
	float factors[][2] = {{1, 1.1f}, {0.9f, 1.02f}, {1.1, 0.8}};
	int colors[] = {videodisplay.RGB(0xff0000), videodisplay.RGB(0x00ff00), videodisplay.RGB(0x0000ff)};
	//animate them with milliseconds
	float p = millis() * 0.002f;
	for (int i = 0; i < 3; i++)
	{
		//calculate the position
		int x = videodisplay.xres / 2 + sin(p * factors[i][0]) * videodisplay.xres / 3;
		int y = videodisplay.yres / 2 + cos(p * factors[i][1]) * videodisplay.yres / 3;
		//clear the center with a black filled circle
		videodisplay.fillCircle(x, y, 8, 0);
		//draw the circle with the color from the array
		videodisplay.circle(x, y, 10, colors[i]);
	}
	//render the flame effect
	for (int y = 0; y < videodisplay.yres - 9; y++)
		for (int x = 1; x < videodisplay.xres - 1; x++)
		{
			//take the avarage from the surrounding pixels below
			int c0 = videodisplay.get(x, y);
			int c1 = videodisplay.get(x, y + 1);
			int c2 = videodisplay.get(x - 1, y + 1);
			int c3 = videodisplay.get(x + 1, y + 1);
			int r = ((c0 & 0x1f) + (c1 & 0x1f) + ((c2 & 0x1f) + (c3 & 0x1f)) / 2) / 3;
			int g = (((c0 & 0x3e0) + (c1 & 0x3e0) + ((c2 & 0x3e0) + (c3 & 0x3e0)) / 2) / 3) & 0x3e0;
			int b = (((c0 & 0x3c00) + (c1 & 0x3c00) + ((c2 & 0x3c00) + (c3 & 0x3c00)) / 2) / 3) & 0x3c00;
			//draw the new pixel
			videodisplay.dotFast(x, y, r | g | b);
		}
}
        */