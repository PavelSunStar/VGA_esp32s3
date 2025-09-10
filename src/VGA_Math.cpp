#include <math.h>
#include "VGA_Math.h"
#include <Arduino.h>
#include "esp_random.h"        // esp_random, esp_fill_random
#include "bootloader_random.h" // bootloader_random_enable, disable

VGA_Math::VGA_Math() {
    //Init LUT
    for (int i = 0; i < LUT_SIZE; ++i) {
        _sinLUT[i] = (int16_t)(1000.0 * sin(i * DEG_TO_RAD));
        _cosLUT[i] = (int16_t)(1000.0 * cos(i * DEG_TO_RAD));
    }
}

void VGA_Math::LUT(int &x, int &y, int xx, int yy, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    x = xx + (len * _cosLUT[angle]) / 1000;
    y = yy + (len * _sinLUT[angle]) / 1000;
}

int VGA_Math::xLUT(int x, int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return x + (len * _cosLUT[angle]) / 1000;
}

int VGA_Math::yLUT(int x, int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return y + (len * _sinLUT[angle]) / 1000;
}

void VGA_Math::startFPS() {
    _frameCount = 0;
    _fpsStartTime = millis();
}

void VGA_Math::calcFPS() {
    _frameCount++;
    unsigned long t = millis();
    if (t - _fpsStartTime >= 1000) {
        _fps = (_frameCount * 1000.0f) / (t - _fpsStartTime);
        _frameCount = 0;
        _fpsStartTime = t;
    }
}

float VGA_Math::getFPS() const {
    return _fps;
}

// одно 32-битное случайное число
uint32_t VGA_Math::randomU32() {
    return esp_random();
}

// случайное число в диапазоне [min, max]
int VGA_Math::randomInt(int min, int max) {
    if (min >= max) return min;
    uint32_t val = randomU32();
    return min + (val % (max - min + 1));
}

// случайное float в диапазоне [0, 1)
float VGA_Math::randomFloat() {
    return (float)randomU32() / (float)UINT32_MAX;
}

// случайное double в диапазоне [0, 1)
double VGA_Math::randomDouble() {
    return (double)randomU32() / (double)UINT32_MAX;
}

// случайное bool (true/false)
bool VGA_Math::randomBool() {
    return (randomU32() & 1) != 0;
}

// случайное float в диапазоне [min, max)
float VGA_Math::randomRangeFloat(float min, float max) {
    return min + (randomFloat() * (max - min));
}

// заполнение буфера случайными байтами
void VGA_Math::randomBytes(void* buf, size_t len) {
    esp_fill_random(buf, len);
}

// включить "истинный" TRNG (SAR ADC источник энтропии)
//При включении enableTrueRandom() — подключается SAR ADC шум, и получаем «настоящие» случайные числа (рекомендовано для безопасности и криптографии).
void VGA_Math::enableTrueRandom() {
    bootloader_random_enable();
}

// выключить TRNG (освободить SAR ADC для Wi-Fi/BT/ADC)
//По умолчанию работает встроенный генератор с осциллятором (хорош для игр, графики, спрайтов).
void VGA_Math::disableTrueRandom() {
    bootloader_random_disable();
}
