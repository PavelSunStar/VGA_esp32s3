#pragma once

#include "../VGA_esp32.h"

void VGA_esp32::setPalCol(uint8_t index, uint8_t r, uint8_t g, uint8_t b){
    if(!_s.inited || !_s.usePal) return;
    
    uint16_t* pal = _s.pal;
    pal += index <<_s.scale;
    uint16_t color = RGB565(r, g, b);
    for (int i = 0; i < scaleMul[_s.scale]; i++) *pal++ = color;
}