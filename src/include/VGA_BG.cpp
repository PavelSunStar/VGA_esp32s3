#include "../VGA_esp32.h"

bool VGA_esp32::initBG(uint16_t width, uint16_t height){
    if (width < _s.width || height < _s.height) return false;

    if (_bg.buf){
        free(_bg.buf);
        _bg = {};
    }

    int size = width * height * ((_s.usePal || _s.bpp == ColorDepth::BPP_8) ? sizeof(uint8_t) : sizeof(uint16_t));
    if (!(_bg.buf = allocateMemory(size, true))) return false;  
    
    _bg.width = width;
    _bg.height = height;

    return (_bg.inited = true);
}

bool VGA_esp32::loadBG(const uint8_t* buffer){
    if (!buffer) return false;

    // Формат изображения
    uint8_t bpp = buffer[0];
    uint8_t count = buffer[1];

    if (count == 0) return false;

    // Проверяем RGB332
    if (bpp != 8) return false;

    // Читаем размеры изображения
    uint16_t width  = buffer[6] | (buffer[7] << 8);
    uint16_t height = buffer[8] | (buffer[9] << 8);

    // Проверяем формат VGA-буфера
    if (!_s.usePal && _s.bpp != ColorDepth::BPP_8) return false;

    // Выделяем память под фон
    if (!initBG(width, height)) return false;

    // Размер изображения в байтах
    size_t size = (size_t)width * height;

    // Копируем картинку из Flash в PSRAM
    memcpy(_bg.buf, buffer + 10, size);

    return true;
}


void VGA_esp32::drawBG(
    int offsetx,
    int offsety,
    int height){

    // ======================================
    // ПРОВЕРКА
    // ======================================

    if (!_bg.inited || !_bg.buf)
        return;

    auto& s = _s;

    const int W = s.width;
    const int H = s.height;

    const int BW = _bg.width;
    const int BH = _bg.height;

    if (W <= 0 || H <= 0 ||
        BW <= 0 || BH <= 0)
        return;

    // ======================================
    // ВЫСОТА ОТРИСОВКИ
    // ======================================

    // -1 = весь экран
    if (height < 0)
        height = H;

    // Ограничиваем высотой экрана
    if (height > H)
        height = H;

    if (height == 0)
        return;

    // ======================================
    // ФОРМАТ ПИКСЕЛЕЙ
    // ======================================

    const int bpp =
        (s.usePal ||
         s.bpp == ColorDepth::BPP_8)
        ? 1 : 2;

    // Размер строки в байтах
    const size_t dstStride = W * bpp;
    const size_t srcStride = BW * bpp;

    // ======================================
    // УКАЗАТЕЛИ НА БУФЕРЫ
    // ======================================

    const uint8_t* src =
        (const uint8_t*)_bg.buf;

    uint8_t* dst =
        (uint8_t*)Buf();

    if (!dst)
        return;

    // ======================================
    // ЦИКЛИЧЕСКИЕ КООРДИНАТЫ
    // ======================================

    // Поддержка отрицательных offset
    offsetx %= BW;
    offsety %= BH;

    if (offsetx < 0)
        offsetx += BW;

    if (offsety < 0)
        offsety += BH;

    // ======================================
    // РАЗМЕРЫ БЛОКОВ КОПИРОВАНИЯ
    // ======================================

    // Сколько пикселей осталось
    // до правого края панорамы

    int firstPixels = BW - offsetx;

    if (firstPixels > W)
        firstPixels = W;

    const size_t firstBytes =
        firstPixels * bpp;

    const size_t xBytes =
        offsetx * bpp;

    // Остаток строки после перехода
    // через правый край панорамы

    const size_t remainBytes =
        dstStride - firstBytes;

    // ======================================
    // НАЧАЛЬНАЯ СТРОКА ПАНОРАМЫ
    // ======================================

    int sy = offsety;

    const uint8_t* srcLine =
        src + sy * srcStride;

    // ======================================
    // БЫСТРЫЙ РЕЖИМ
    // Панорама не пересекает край по X
    // ======================================

    if (remainBytes == 0){

        for (int y = 0; y < height; y++){

            memcpy(
                dst,
                srcLine + xBytes,
                dstStride
            );

            // Следующая строка экрана
            dst += dstStride;

            // Следующая строка панорамы
            sy++;

            if (sy >= BH){

                sy = 0;
                srcLine = src;

            } else {

                srcLine += srcStride;
            }
        }

        return;
    }

    // ======================================
    // ЦИКЛИЧЕСКОЕ КОПИРОВАНИЕ
    // Панорама пересекает край по X
    // ======================================

    for (int y = 0; y < height; y++){

        // Первая часть строки
        memcpy(
            dst,
            srcLine + xBytes,
            firstBytes
        );

        // Оставшаяся часть строки
        // с начала панорамы

        size_t copied = firstBytes;
        size_t remain = remainBytes;

        while (remain){

            size_t bytes = remain;

            if (bytes > srcStride)
                bytes = srcStride;

            memcpy(
                dst + copied,
                srcLine,
                bytes
            );

            copied += bytes;
            remain -= bytes;
        }

        // Следующая строка экрана
        dst += dstStride;

        // Следующая строка панорамы
        sy++;

        if (sy >= BH){

            sy = 0;
            srcLine = src;

        } else {

            srcLine += srcStride;
        }
    }
}


