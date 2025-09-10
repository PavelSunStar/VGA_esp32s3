#pragma once

#include <cstdint>  // для uint32_t
#include <cstddef>  // для size_t

#define LUT_SIZE 360

class VGA_Math {
public:
    VGA_Math();

    void LUT(int &x, int &y, int xx, int yy, int len, int angle);
    int xLUT(int x, int y, int len, int angle);
    int yLUT(int x, int y, int len, int angle);

    void startFPS();
    void calcFPS();
    float getFPS() const;

    // --- Random Numbers ---
    static uint32_t randomU32();
    static int randomInt(int min, int max);
    static float randomFloat();
    static double randomDouble();
    static bool randomBool();
    static float randomRangeFloat(float min, float max);

    // --- Random Bytes ---
    static void randomBytes(void* buf, size_t len);

    // --- True RNG control ---
    static void enableTrueRandom();
    static void disableTrueRandom(); 
    
    protected:
        int16_t _sinLUT[LUT_SIZE];
        int16_t _cosLUT[LUT_SIZE];

        unsigned long _frameCount = 0;
        unsigned long _fpsStartTime = 0;
        float _fps = 0.0f;

};
