#include "VGA_esp32.h"

VGA_esp32::VGA_esp32(){
    loadFont(FONT_8x8_Def);
}

VGA_esp32::~VGA_esp32(){
    destroy();
}

void* VGA_esp32::allocateMemory(size_t size, bool psram){
    const size_t alignment = psram ? 64 : 4;

    uint32_t caps = (psram ? MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT : MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    void* ptr = heap_caps_aligned_alloc(alignment, size, caps);
    if (!ptr){
        Serial.printf(
            "\n[MEMORY] Memory allocation FAILED: %u bytes (%s)\n",
            (unsigned)size,
            psram ? "PSRAM" : "SRAM"
        );

        return nullptr;
    }
    memset(ptr, 0, size);

    Serial.printf(
        "[MEMORY] Allocated %u bytes in %s, align=%u, ptr=%p\n",
        (unsigned)size,
        psram ? "PSRAM" : "SRAM",
        (unsigned)alignment,
        ptr
    );

    return ptr;
}

bool VGA_esp32::init(ScreenMode m, ColorDepth cd, bool usePal, bool dBuff){
    destroy();

    Serial.println("\n[VGA] Starting...");
    if (!setMode(m)) return false;

    _s.usePal = usePal;
    _s.dBuff = dBuff;
    _s.bpp = (_s.usePal ? ColorDepth::BPP_8 : cd);
    setScreenDimentions();

    if (!getMemory()) return false;
    setBufferAddr();

    if (!setRGBPanel()) return false;

    if (_s.usePal || _s.dBuff) {
        if (!regSemaphore()) return (_s.inited = false);
    } else {
        _sem_vsync_end = nullptr;
        _sem_gui_ready = nullptr;
    }
    regCallBack();

    if (!initPanel()) return false;

    return (_s.inited = true);
}

bool IRAM_ATTR VGA_esp32::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;
    auto& f = vga->_f;

    f.frame++;

    return false;
}

bool IRAM_ATTR VGA_esp32::on_bounce_empty(esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;

    auto& s = vga->_s;
    auto& sp = vga->_sp;

    if (s.scale == 0){
        uint8_t* dst = (uint8_t*)bounce_buf;
        uint8_t* src = (uint8_t*)s.fb0 + (pos_px << sp.shift);
        memcpy(dst, src, len_bytes);
    } else {
        int lines       = sp.lines;
        int pixels      = sp.pixels;
        int copyBytes   = sp.copyBytes;
        int skip        = sp.skip;

        uint32_t color;
        uint32_t* savePos   = nullptr;
        uint32_t* dst       = (uint32_t*)bounce_buf;

        if (s.scale == 1){            
            if (s.bpp == ColorDepth::BPP_16){
                uint16_t* src = (uint16_t*)s.fb0 + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dst;

                    for (int i = 0; i < pixels; i++){
                        color = *src++; *dst++ = color | (color << 16);
                        color = *src++; *dst++ = color | (color << 16);
                        color = *src++; *dst++ = color | (color << 16);
                        color = *src++; *dst++ = color | (color << 16);
                    }   

                    memcpy(dst, savePos, copyBytes);
                    dst += skip;
                }                 
            } else {
                uint8_t* src = (uint8_t*)s.fb0 + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dst;

                    for (int i = 0; i < pixels; i++){
                        color = *src++; color |= (*src++ << 16); color |= color << 8; *dst++ = color;
                        color = *src++; color |= (*src++ << 16); color |= color << 8; *dst++ = color;
                    }  

                    memcpy(dst, savePos, copyBytes);
                    dst += skip;
                } 
            }
        } else {
            int skip2       = skip << 1;
            int copyBytes2x = sp.copyBytes2x;

            if (s.bpp == ColorDepth::BPP_16){
                uint16_t* src = (uint16_t*)s.fb0 + (pos_px >> 4);  
              
                while (lines-- > 0){
                    savePos = dst;

                    for (int i = 0; i < pixels; i++){
                        color = *src++; color |= (color << 16); *dst++ = color; *dst++ = color;
                        color = *src++; color |= (color << 16); *dst++ = color; *dst++ = color;
                    }    

                    memcpy(dst, savePos, copyBytes);   dst += skip;
                    memcpy(dst, savePos, copyBytes2x); dst += skip2;
                }                
            } else {
                uint8_t* src = (uint8_t*)s.fb0 + (pos_px >> 4);

                while (lines-- > 0){
                    savePos = dst;

                    for (int i = 0; i < pixels; i++){
                        color = *src++; color |= (color << 16); color |= color << 8; *dst++ = color;
                        color = *src++; color |= (color << 16); color |= color << 8; *dst++ = color;
                    }    

                    memcpy(dst, savePos, copyBytes);   dst += skip;
                    memcpy(dst, savePos, copyBytes2x); dst += skip2;
                }                
            }
        }
    }    

    BaseType_t hp_task_woken = pdFALSE;
    if (s.dBuff && pos_px >= sp.lastPos){
        if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, &hp_task_woken) == pdTRUE){
            std::swap(s.fb0, s.fb1);

            if (s.bpp == ColorDepth::BPP_8)
                std::swap(s.fLine8, s.bLine8);
            else
                std::swap(s.fLine16, s.bLine16);

            xSemaphoreGiveFromISR(vga->_sem_vsync_end, &hp_task_woken);
        }
    }

    return hp_task_woken == pdTRUE;
}

bool IRAM_ATTR VGA_esp32::on_bounce_empty_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;

    auto& s = vga->_s;
    auto& sp = vga->_sp;
    uint32_t* dest = (uint32_t*)bounce_buf;

    if (s.scale == 0){
        uint16_t* pal16 = (uint16_t*)s.pal;
        uint8_t* sour = (uint8_t*)s.fb0 + pos_px;

        int loops = len_bytes >> 4; // 8 pixels per loop

        while (loops--){
            uint32_t c0, c1;

            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
        }
    } else if (s.scale == 1){
        const uint32_t* pal32 = (const uint32_t*)s.pal;
        const uint8_t* sour = (const uint8_t*)s.fb0 + (pos_px >> 2);

        int lines = sp.lines;
        int pixels = s.width >> 3;       // 320 / 8 = 40
        int copyBytes = s.width << 2;   // 320 * 4 = 1280
        int skip = copyBytes >> 2;

        while (lines-- > 0){
            uint32_t* savePos = dest;

            for (int i = 0; i < pixels; i++){
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
            }

            memcpy(dest, savePos, copyBytes);
            dest += skip;
        }
    } else {
        const uint64_t* pal64 = (const uint64_t*)s.pal;
        const uint8_t* sour = (const uint8_t*)s.fb0 + (pos_px >> 4);

        int lines = sp.lines;
        int pixels = s.width >> 3;       // 160 / 8 = 20
        int copyBytes = s.width << 3;    // 160 * 8 = 1280
        int copyBytes2x = copyBytes << 1;
        int skip = copyBytes >> 2;       // 320 uint32_t
        int skip2x = skip << 1;

        while (lines-- > 0){
            uint64_t* savePos = (uint64_t*)dest;
            uint64_t* dst64 = (uint64_t*)dest;

            for (int i = 0; i < pixels; i++){
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
                *dst64++ = pal64[*sour++];
            }

            dest = (uint32_t*)dst64;

            // Копируем первую строку ещё 3 раза
            memcpy(dest, savePos, copyBytes);
            dest += skip;

            memcpy(dest, savePos, copyBytes2x);
            dest += skip2x;
        }        
    }
    
    BaseType_t hp_task_woken = pdFALSE;
    if (s.dBuff && pos_px >= sp.lastPos){
        if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, &hp_task_woken) == pdTRUE){
            std::swap(s.fb0, s.fb1);

            if (s.bpp == ColorDepth::BPP_8)
                std::swap(s.fLine8, s.bLine8);
            else
                std::swap(s.fLine16, s.bLine16);

            xSemaphoreGiveFromISR(vga->_sem_vsync_end, &hp_task_woken);
        }
    }

    return hp_task_woken == pdTRUE;    
}

void VGA_esp32::swap(){
    if (_s.dBuff && _sem_gui_ready && _sem_vsync_end){
        xSemaphoreGive(_sem_gui_ready);
        xSemaphoreTake(_sem_vsync_end, portMAX_DELAY);
    }
}

void VGA_esp32::updateFPS(){
    uint32_t now = millis();
    uint32_t elapsed = now - _f.fpsStartTime;
    _f.frameCount++;

    if (elapsed >= 1000){
        uint32_t frames = _f.frameCount;
        _f.fps = (uint32_t)(frames - _f.lastFrameCount) * 1000.0f / elapsed;
        _f.lastFrameCount = frames;
        _f.fpsStartTime = now;
    }
}
