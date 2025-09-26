#include <Arduino.h>
#include "VGA_esp32s3.h"
#include "esp_heap_caps.h"
#include <esp_LCD_panel_ops.h>

// Конструктор: инициализация последовательного порта для отладки
VGA_esp32s3::VGA_esp32s3() {
    Serial.begin(115200);
}

// Деструктор: освобождение ресурсов
VGA_esp32s3::~VGA_esp32s3() {
    if (_buf16) {
        heap_caps_free(_buf16);
        _buf16 = nullptr;
    }

    if (_buf8) {
        heap_caps_free(_buf8);
        _buf8 = nullptr;
    }

    if (_tmpBuf) {
        heap_caps_free(_tmpBuf);
        _tmpBuf = nullptr;
    }

    free(_fastY); _fastY = nullptr;
}

void VGA_esp32s3::regCallbackSemaphore() {
    esp_lcd_rgb_panel_event_callbacks_t cbs;
        cbs.on_bounce_empty = on_bounce_empty;
        cbs.on_color_trans_done = on_color_trans_done;
        cbs.on_vsync = on_vsync;
        cbs.on_bounce_frame_finish = on_bounce_frame_finish;
        cbs.on_frame_buf_complete = on_frame_buf_complete;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(_panel_handle, &cbs, this));

    if (_dBuff){
        _sem_vsync_end = xSemaphoreCreateBinary();
        assert(_sem_vsync_end);

        _sem_gui_ready = xSemaphoreCreateBinary();
        assert(_sem_gui_ready);
    }
}

void VGA_esp32s3::printInfo(){
    Serial.printf("Mode: %dx%dx%d, Double buffer: %s, PSRam: %s\n", 
        _width, _height, _colBit,  
        _dBuff ? "yes" : "no",
        _psRam ? "yes" : "no");
    Serial.printf("Screen mode: %dx%dx%d, Scale: %d\n", 
        _scrWidth, _scrHeight, _bpp, _scale); 
    Serial.printf("Viewport: (%d, %d - %d, %d)\n", _vX1, _vY1, _vX2, _vY2);
    Serial.printf("Buffer address: %p, Buffer size: %d\n", ((_bpp == 16) ? (void*)_buf16 : (void*)_buf8), ((_dBuff) ? _scrFullSize << 1 : _scrFullSize)); 
    Serial.printf("Screen size: %d, Block size: %d\n", _scrFullSize, _bounce_buffer_size_px);
}

// Настройка конфигурации RGB-панели
bool VGA_esp32s3::setPanelConfig() {
    esp_lcd_rgb_panel_config_t panel_config;
    memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

    // Настройка пинов для 8-битного или 16-битного режима
    if (_bpp == 16) {
        // 16-битный режим (RGB565)        
        panel_config.data_gpio_nums[0]  = _pins[15]; // B
        panel_config.data_gpio_nums[1]  = _pins[14];
        panel_config.data_gpio_nums[2]  = _pins[13];
        panel_config.data_gpio_nums[3]  = _pins[12];
        panel_config.data_gpio_nums[4]  = _pins[11];

        panel_config.data_gpio_nums[5]  = _pins[10]; // G
        panel_config.data_gpio_nums[6]  = _pins[9];
        panel_config.data_gpio_nums[7]  = _pins[8];
        panel_config.data_gpio_nums[8]  = _pins[7];
        panel_config.data_gpio_nums[9]  = _pins[6];
        panel_config.data_gpio_nums[10] = _pins[5];

        panel_config.data_gpio_nums[11] = _pins[4];  // R
        panel_config.data_gpio_nums[12] = _pins[3];
        panel_config.data_gpio_nums[13] = _pins[2];
        panel_config.data_gpio_nums[14] = _pins[1];
        panel_config.data_gpio_nums[15] = _pins[0];
    } else {
        // 8-битный режим
        panel_config.data_gpio_nums[0] = _pins[12]; // B
        panel_config.data_gpio_nums[1] = _pins[11];
        panel_config.data_gpio_nums[2] = _pins[7];  // G
        panel_config.data_gpio_nums[3] = _pins[6];
        panel_config.data_gpio_nums[4] = _pins[5];
        panel_config.data_gpio_nums[5] = _pins[2];  // R
        panel_config.data_gpio_nums[6] = _pins[1];
        panel_config.data_gpio_nums[7] = _pins[0];
    }

    // Настройка управляющих сигналов
    panel_config.disp_gpio_num  = VGA_PIN_NUM_DISP_EN;  // Не используется
    panel_config.pclk_gpio_num  = VGA_PIN_NUM_PCLK;     // Не используется
    panel_config.de_gpio_num    = VGA_PIN_NUM_DE;       // Не используется
    panel_config.hsync_gpio_num = _pins[16];    // HSYNC
    panel_config.vsync_gpio_num = _pins[17];    // VSYNC

    // Тайминги для VGA из VGA_esp32s3.h)dma_burst_size
    panel_config.clk_src                    = LCD_CLK_SRC_PLL240M;  // Источник тактового сигнала
    panel_config.timings.pclk_hz            = _pclk_hz;             // Частота пиксельного сигнала 25 МГц
    panel_config.timings.h_res              = _width;               // Горизонтальное разрешение
    panel_config.timings.v_res              = _height;              // Вертикальное разрешение
    panel_config.timings.hsync_back_porch   = _hsync_back_porch;    // Задний порог HSYNC
    panel_config.timings.hsync_front_porch  = _hsync_front_porch;   // Передний порог HSYNC
    panel_config.timings.hsync_pulse_width  = _hsync_pulse_width;   // Ширина импульса HSYNC
    panel_config.timings.vsync_back_porch   = _vsync_back_porch;    // Задний порог VSYNC
    panel_config.timings.vsync_front_porch  = _vsync_front_porch;   // Передний порог VSYNC
    panel_config.timings.vsync_pulse_width  = _vsync_pulse_width;   // Ширина импульса VSYNC

    // ---------------- Режим "ручной буфер" ----------------
    panel_config.data_width             = _colBit;
    panel_config.bits_per_pixel         = _bpp;
    panel_config.bounce_buffer_size_px  = _bounce_buffer_size_px;
    panel_config.sram_trans_align       = 4;
    panel_config.psram_trans_align      = 64;
    panel_config.flags.fb_in_psram      = false;
    panel_config.num_fbs                = 0;
    panel_config.flags.no_fb            = true;

    panel_config.timings.flags.pclk_active_neg = true;//+
    panel_config.timings.flags.hsync_idle_low = false;
    panel_config.timings.flags.vsync_idle_low = false;


    // Создание RGB-панели
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to create RGB panel: %s\n", esp_err_to_name(ret));
        return false;
    }

    // Сброс и инициализация
    ret = esp_lcd_panel_reset(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(ret));
        return false;
    }
    ret = esp_lcd_panel_init(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(ret));
        return false;
    }

    // --- Свои буферы ---
    uint32_t caps = MALLOC_CAP_DMA | (_psRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_INTERNAL) | MALLOC_CAP_8BIT;
    if (_bpp == 16){
        _buf16 = (uint16_t*)heap_caps_malloc((_dBuff ? (_scrFullSize << 1) : _scrFullSize) , caps);
        assert(_buf16);
        memset(_buf16, 0, (_dBuff ? _scrFullSize << 1: _scrFullSize));
    } else {
        _buf8 = (uint8_t*)heap_caps_malloc((_dBuff ? (_scrFullSize << 1) : _scrFullSize) , caps);
        assert(_buf8);
        memset(_buf8, 0, (_dBuff ? _scrFullSize << 1: _scrFullSize));
    }
    
    //Temp buffer
    caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT; // внутреняя RAM всегда
    _tmpBufSize = _scrWidth * 10 * ((_colBit == 16) ? sizeof(uint16_t) : sizeof(uint8_t));
    _tmpBuf = (uint8_t*)heap_caps_malloc(_tmpBufSize, caps);
    assert(_tmpBuf);
    memset(_tmpBuf, 0, _tmpBufSize);

    regCallbackSemaphore();
    esp_lcd_panel_disp_on_off(_panel_handle, true);

    printInfo();
    Serial.println("Init...Ok\n");

    _fastY = (int*) malloc(_scrHeight * sizeof(int));
    for (int y = 0; y < _scrHeight; y++) {
        _fastY[y] = (_scrWidth * y);
    }

    return true;
}

//*****************************************************************************************/
void VGA_esp32s3::setPins(uint8_t r0, uint8_t r1, uint8_t r2, uint8_t r3, uint8_t r4, 
                          uint8_t g0, uint8_t g1, uint8_t g2, uint8_t g3, uint8_t g4, uint8_t g5,
                          uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                          uint8_t h_sync, uint8_t v_sync){
    
    //Red pins
    _pins[0] = r0;
    _pins[1] = r1;
    _pins[2] = r2;
    _pins[3] = r3;
    _pins[4] = r4;

    //Green pins
    _pins[5] = g0;
    _pins[6] = g1;
    _pins[7] = g2;
    _pins[8] = g3;
    _pins[9] = g4;
    _pins[10] = g5;

    //Blue pins
    _pins[11] = b0;
    _pins[12] = b1;
    _pins[13] = b2;
    _pins[14] = b3;
    _pins[15] = b4;

    //H_Sync, V_Sync
    _pins[16] = h_sync;
    _pins[17] = v_sync;
}                     

// Инициализация дисплея
bool VGA_esp32s3::init(const int *mode, int scale, bool dBuff, bool psRam) {
    if (!mode) {
        Serial.println("ERROR: Mode array is null");
        return false;
    }

// Отладочный вывод-----------------------------------------------------------------
    Serial.println("\n[=== Init VGA ===]");

    // Извлечение параметров режима
    _pclk_hz                = mode[0];
    _width                  = mode[1]; 
    _height                 = mode[2];
    _colBit                 = mode[3]; _colBit = (_colBit > 8) ? 16 : 8;
    _bpp                    = mode[4]; _bpp = (_bpp >= _colBit) ?  _colBit: _bpp; 
    _hsync_front_porch      = mode[5];
    _hsync_pulse_width      = mode[6];
    _hsync_back_porch       = mode[7];
    _vsync_front_porch      = mode[8];
    _vsync_pulse_width      = mode[9];
    _vsync_back_porch       = mode[10];
    _bounce_buffer_size_px  = mode[11]; //_height / 10 * _width;//mode[12];

    _scale = (scale >= 2) ? 2 : (scale == 1 ? 1 : 0);
    _bppShift = (_colBit == 16 ? 1 : 0);
    _psRam = psRam;
    _dBuff = dBuff;
    _size = _width * _height;

    _scrWidth = _width >> _scale;
    _scrHeight = _height >> _scale;
    _aspect = (float)_scrWidth / _scrHeight;
    _scrCX = _scrWidth >> 1;
    _scrCY = _scrHeight >> 1;    
    _scrXX = _scrWidth - 1;
    _scrYY = _scrHeight - 1;
    _scrSize = _scrWidth * _scrHeight;
    _scrFullSize = _scrSize << _bppShift;

    _frontBuff = 0;
    _backBuff = (_dBuff) ? _scrFullSize : 0;

    _lastBounceBufferPos = _size - _bounce_buffer_size_px;
    _tik = _width >> 4;
    _lines = (_bounce_buffer_size_px / _width) >> _scale;
    _width2X = _width << 1;
    _width4X = _width2X << 1;
    _scrWidth2X = _scrWidth << 1;
    _scrWidth4X = _scrWidth2X << 1;
    setViewport(0, 0, _scrXX, _scrYY);

    if (!setPanelConfig()) return false;

    return true;
}

//CallBack's*****************************************************
bool IRAM_ATTR VGA_esp32s3::on_bounce_empty(
    esp_lcd_panel_handle_t panel,
    void *bounce_buf,
    int pos_px,
    int len_bytes,
    void *user_ctx){

    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;
    int lines = vga->_lines;

    if (vga->BPP() == 16){
        uint16_t* dest = (uint16_t*)bounce_buf;
        uint16_t* sour = PTR_OFFSET_T(vga->_buf16, vga->_frontBuff, uint16_t); 

        if (vga->Scale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->Scale() == 1){
            sour += pos_px >> 2;

            while (lines-- > 0){
                int tik = vga->_tik;
                uint16_t* savePos = dest;

                while (tik-- > 0){
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                }

                memcpy(dest, savePos, vga->_width2X);
                dest += vga->_width;  
            }    
        } else {
            sour += pos_px >> 4;

            while (lines-- > 0){
                int tik = vga->_tik;
                uint16_t* savePos = dest;

                while (tik-- > 0){
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                }

                memcpy(dest, savePos, vga->_width2X); 
                dest += vga->_width;
                memcpy(dest, savePos, vga->_width4X); 
                dest += vga->_width2X;
            }     
        }
    } else {
        uint8_t* dest = (uint8_t*)bounce_buf;
        uint8_t* sour = PTR_OFFSET_T(vga->_buf8, vga->_frontBuff, uint8_t);

        if (vga->Scale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->Scale() == 1){
            sour += pos_px >> 2;

            while (lines-- > 0){
                int tik = vga->_tik;
                uint8_t* savePos = dest;

                while (tik-- > 0){
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++); 
                    *(dest++) = *sour; *(dest++) = *(sour++); *(dest++) = *sour; *(dest++) = *(sour++);                     
                }

                memcpy(dest, savePos, vga->_width);
                dest += vga->_width;                
            }
        } else {
            sour += pos_px >> 4;
            while (lines-- > 0){
                int tik = vga->_tik;
                uint8_t* savePos = dest;

                while (tik-- > 0){
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                    *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *sour; *(dest++) = *(sour++);  
                }

                memcpy(dest, savePos, vga->_width); 
                dest += vga->_width;
                memcpy(dest, savePos, vga->_width2X); 
                dest += vga->_width2X;
            }                
        }
    }

    // Последний bounce → пробуем сменить буфер
    if (pos_px >= vga->_lastBounceBufferPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, NULL) == pdTRUE) {
            std::swap(vga->_frontBuff, vga->_backBuff);
            xSemaphoreGiveFromISR(vga->_sem_vsync_end, NULL);
        }
    }        
 
    return true;
}

bool IRAM_ATTR VGA_esp32s3::on_color_trans_done(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {    

    return false; 
}

bool IRAM_ATTR VGA_esp32s3::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;

    vga->_timer++;
    
    return true;
}

bool IRAM_ATTR VGA_esp32s3::on_bounce_frame_finish(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {    
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;

    return false; 
}

bool IRAM_ATTR VGA_esp32s3::on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {    
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;

    return false; 
}

void VGA_esp32s3::setViewport(int x1, int y1, int x2, int y2) {
    if (x1 == x2 || y1 == y2) return;

    // Обеспечение x1 <= x2 и y1 <= y2
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Проверка на нулевую или некорректную область
    x1 = std:: max(0, x1);
    y1 = std:: max(0, y1);
    x2 = std:: min(_scrXX, x2);
    y2 = std:: min(_scrYY, y2);

    // Вычисление ширины и высоты области просмотра
    _vX1 = x1;
    _vY1 = y1;
    _vX2 = x2;
    _vY2 = y2;
    _vWidth = _vX2 - _vX1 + 1;
    _vHeight = _vY2 - _vY1 + 1;
    _vCX = _vWidth >> 1;
    _vCY = _vHeight >> 1;
    _vXX = _vWidth - 1;
    _vYY = _vHeight - 1;
} 

void VGA_esp32s3::swap() {
    if (_dBuff){
        xSemaphoreGive(_sem_gui_ready);
        xSemaphoreTake(_sem_vsync_end, portMAX_DELAY);
    }
}

//===[Screen scroll]===
void VGA_esp32s3::scrollX(int sx){
    sx %= _scrWidth;
    if (sx == 0) return;

    uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _backBuff, uint8_t));

    int sizeY = _scrHeight;
    int copyBytes = abs(sx) << _bppShift;
    int copyBlock = (_scrWidth - abs(sx)) << _bppShift;
    int skip = _scrWidth << _bppShift;

    if (sx > 0){// влево  
        int copyBytes = sx << _bppShift; 

        while (sizeY-- > 0){                
            memcpy(_tmpBuf, scr + copyBytes, copyBlock);
            memcpy(scr + copyBlock, scr, copyBytes);
            memcpy(scr, _tmpBuf, copyBlock);
            scr += skip;
        }    
    } else {// вправо            
        int shift = abs(sx);
        int copyRight = _scrWidth - shift;

        while (sizeY-- > 0) {
            memcpy(_tmpBuf, scr, copyBlock);
            memcpy(scr, scr + copyBlock, copyBytes);
            memcpy(scr + copyBytes, _tmpBuf, copyBlock);
            scr += skip;  
        }
    }
}

void VGA_esp32s3::scrollY(int sy) {
    if (sy == 0 || sy % _scrHeight == 0) return;

    // ограничиваем сдвиг буфером на 10 строк
    sy = std::clamp(sy, -10, 10);

    uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _backBuff, uint8_t));

    int sizeY = _scrHeight - abs(sy);
    int lineSize = _scrWidth << _bppShift;
    int copyBytes = lineSize * abs(sy);

    if (sy > 0) { // вверх
        memcpy(_tmpBuf, scr, copyBytes);
        while (sizeY-- > 0){
            memcpy(scr, scr + copyBytes, lineSize);
            scr += lineSize;
        }
        memcpy(scr, _tmpBuf, copyBytes);
    } else { // вниз
        uint8_t* savePos = scr;
        scr += _scrFullSize - copyBytes - lineSize;
        memcpy(_tmpBuf, scr, copyBytes);

        while (sizeY-- > 0){
            memcpy(scr + copyBytes, scr, lineSize);
            scr -= lineSize;
        }
            
        memcpy(savePos, _tmpBuf, copyBytes);
    }
}

void VGA_esp32s3::scrollXY(int sx, int sy){
    sx %= _scrWidth; 
    sy = std::clamp(sy, -10, 10);
    if (sx == 0 && sy == 0) return;

    uint8_t* scrX = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _backBuff, uint8_t));
    uint8_t* scrY = scrX;                                    

    if (sx != 0){
        int sizeY = _scrHeight;        
        int copyBytes = abs(sx) << _bppShift;
        int copyBlock = (_scrWidth - abs(sx)) << _bppShift;
        int skip = _scrWidth << _bppShift;

        if (sx > 0){
            while (sizeY-- > 0){
                memcpy(_tmpBuf, scrX + copyBytes, copyBlock);
                memcpy(scrX + copyBlock, scrX, copyBytes);
                memcpy(scrX, _tmpBuf, copyBlock);
                scrX += skip;
            }
        } else {
            while (sizeY-- > 0){
                memcpy(_tmpBuf, scrX, copyBlock);
                memcpy(scrX, scrX + copyBlock, copyBytes);
                memcpy(scrX + copyBytes, _tmpBuf, copyBlock);
                scrX += skip;                
            }
        }
    }

    if (sy != 0){
        int sizeY = _scrHeight - abs(sy);
        int lineSize = _scrWidth << _bppShift;
        int copyBytes = lineSize * abs(sy);
        
        if (sy > 0) { // вверх
            memcpy(_tmpBuf, scrY, copyBytes);
            while (sizeY-- > 0){
                memcpy(scrY, scrY + copyBytes, lineSize);
                scrY += lineSize;
            }
            memcpy(scrY, _tmpBuf, copyBytes);
        } else { // вниз
            uint8_t* savePos = scrY;
            scrY += _scrFullSize - copyBytes - lineSize;
            memcpy(_tmpBuf, scrY, copyBytes);

            while (sizeY-- > 0){
                memcpy(scrY + copyBytes, scrY, lineSize);
                scrY -= lineSize;
            }
            
            memcpy(savePos, _tmpBuf, copyBytes);
        }
    }
}

//===[Windows scroll]===
void VGA_esp32s3::winScrollX(int x1, int y1, int x2, int y2, int sx){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(_scrXX, x2);
    y2 = std::min(_scrYY, y2); 
    
    int width  = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if (width < 2 || height < 2) return; 

    sx %= width;
    if (sx == 0) return;

    uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _backBuff, uint8_t));
    scr += (*(_fastY + y1) + x1) << _bppShift;

    int sizeY = height;
    int copyBytes = abs(sx) << _bppShift;
    int copyBlock = (width - abs(sx)) << _bppShift;
    int skip = _scrWidth << _bppShift;

    if (sx > 0){// влево  
        while (sizeY-- > 0){                
            memcpy(_tmpBuf, scr, copyBytes);
            memmove(scr, scr + copyBytes, copyBlock);
            memcpy(scr + copyBlock, _tmpBuf, copyBytes);
            scr += skip;
        }    
    } else {// вправо   
        while (sizeY-- > 0) {
            memcpy(_tmpBuf, scr + copyBlock, copyBytes);
            memmove(scr + copyBytes, scr, copyBlock);
            memcpy(scr, _tmpBuf, copyBytes);
            scr += skip;  
        }
    }   
}

void VGA_esp32s3::winScrollY(int x1, int y1, int x2, int y2, int sy){
    if (sy == 0) return;
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(_scrXX, x2);
    y2 = std::min(_scrYY, y2); 
    
    int width  = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if (width < 2 || height < 2) return; 

    sy = std::clamp(sy, -10, 10);
    uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _backBuff, uint8_t));
    scr += (*(_fastY + y1) + x1) << _bppShift;
    uint8_t* saveBuf = _tmpBuf;
    uint8_t* saveScr = scr;

    int sizeY = height;
    int imgLineSize = width << _bppShift;
    int scrLineSize = _scrWidth << _bppShift;
    int skipImg = (_scrWidth - x2 + x1) << _bppShift;

    if (sy > 0) { // вверх
        int copyY = sy;
        while (copyY-- > 0){
            memcpy(saveBuf, saveScr, imgLineSize);
            saveBuf += imgLineSize;
            saveScr += scrLineSize;
        }

        saveScr = scr;
        copyY = sizeY - sy;
        int skip = sy * scrLineSize;
        while (copyY-- > 0){
            memcpy(saveScr, saveScr + skip, imgLineSize);
            saveScr += scrLineSize;            
        }

        saveBuf = _tmpBuf;
        while (sy-- > 0){
            memcpy(saveScr, saveBuf, imgLineSize);
            saveBuf += imgLineSize;
            saveScr += scrLineSize;            
        }
    } else { // вниз
        sy = -sy;

        // 1. Сохраняем нижние sy строк
        int copyY = sy;
        saveScr += (height - sy) * scrLineSize; // начало "сохраняемого" блока
        while (copyY-- > 0) {
            memcpy(saveBuf, saveScr, imgLineSize);
            saveBuf += imgLineSize;
            saveScr += scrLineSize;
        }

        // 2. Сдвигаем оставшиеся строки вниз
        saveScr = scr + (height - 1) * scrLineSize; // с конца
        copyY = sizeY - sy;
        int skip = sy * scrLineSize;
        while (copyY-- > 0) {
            memcpy(saveScr, saveScr - skip, imgLineSize);
            saveScr -= scrLineSize;
        }

        // 3. Вставляем сохранённые sy строк сверху
        saveBuf = _tmpBuf;
        while (sy-- > 0) {
            memcpy(scr, saveBuf, imgLineSize);
            saveBuf += imgLineSize;
            scr += scrLineSize;
        }    
    }    
}

void VGA_esp32s3::winScrollXY(int x1, int y1, int x2, int y2, int sx, int sy) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(_scrXX, x2);
    y2 = std::min(_scrYY, y2);

    int width  = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if (width < 2 || height < 2) return;

    // --- Скролл по X ---
    sx %= width;
    if (sx != 0) {
        uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t)
                                      : PTR_OFFSET_T(_buf8, _backBuff, uint8_t));
        scr += (*(_fastY + y1) + x1) << _bppShift;

        int sizeY = height;
        int copyBytes = abs(sx) << _bppShift;
        int copyBlock = (width - abs(sx)) << _bppShift;
        int skip = _scrWidth << _bppShift;

        if (sx > 0) { // влево
            while (sizeY-- > 0) {
                memcpy(_tmpBuf, scr, copyBytes);
                memmove(scr, scr + copyBytes, copyBlock);
                memcpy(scr + copyBlock, _tmpBuf, copyBytes);
                scr += skip;
            }
        } else { // вправо
            while (sizeY-- > 0) {
                memcpy(_tmpBuf, scr + copyBlock, copyBytes);
                memmove(scr + copyBytes, scr, copyBlock);
                memcpy(scr, _tmpBuf, copyBytes);
                scr += skip;
            }
        }
    }

    // --- Скролл по Y ---
    if (sy != 0) {
        sy = std::clamp(sy, -10, 10);
        uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _backBuff, uint8_t)
                                      : PTR_OFFSET_T(_buf8, _backBuff, uint8_t));
        scr += (*(_fastY + y1) + x1) << _bppShift;

        uint8_t* saveBuf = _tmpBuf;
        uint8_t* saveScr = scr;

        int sizeY = height;
        int imgLineSize = width << _bppShift;
        int scrLineSize = _scrWidth << _bppShift;

        if (sy > 0) { // вверх
            int copyY = sy;
            while (copyY-- > 0) {
                memcpy(saveBuf, saveScr, imgLineSize);
                saveBuf += imgLineSize;
                saveScr += scrLineSize;
            }

            saveScr = scr;
            copyY = sizeY - sy;
            int skip = sy * scrLineSize;
            while (copyY-- > 0) {
                memcpy(saveScr, saveScr + skip, imgLineSize);
                saveScr += scrLineSize;
            }

            saveBuf = _tmpBuf;
            while (sy-- > 0) {
                memcpy(saveScr, saveBuf, imgLineSize);
                saveBuf += imgLineSize;
                saveScr += scrLineSize;
            }
        } else { // вниз
            sy = -sy;

            // 1. Сохраняем нижние sy строк
            int copyY = sy;
            saveScr += (height - sy) * scrLineSize;
            while (copyY-- > 0) {
                memcpy(saveBuf, saveScr, imgLineSize);
                saveBuf += imgLineSize;
                saveScr += scrLineSize;
            }

            // 2. Сдвигаем оставшиеся строки вниз
            saveScr = scr + (height - 1) * scrLineSize;
            copyY = sizeY - sy;
            int skip = sy * scrLineSize;
            while (copyY-- > 0) {
                memcpy(saveScr, saveScr - skip, imgLineSize);
                saveScr -= scrLineSize;
            }

            // 3. Вставляем сохранённые sy строк сверху
            saveBuf = _tmpBuf;
            while (sy-- > 0) {
                memcpy(scr, saveBuf, imgLineSize);
                saveBuf += imgLineSize;
                scr += scrLineSize;
            }
        }
    }
}

void VGA_esp32s3::copyScr(bool backToFront){
    if (!_dBuff) return;

    uint8_t* scr = (_colBit == 16 ? PTR_OFFSET_T(_buf16, _frontBuff, uint8_t) :
                                    PTR_OFFSET_T(_buf8, _frontBuff, uint8_t));

    if (backToFront){
        memcpy(scr, scr + _backBuff, _scrFullSize);
    } else {
        memcpy(scr + _backBuff, scr, _scrFullSize);
    }
}

void VGA_esp32s3::copyScrRect(int x, int y, int x1, int y1, int x2, int y2, bool backToFront){
    if (!_dBuff) return;

    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 > _scrXX || y1 > _scrYY || x2 < 0 || y2 < 0) return; 

    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(_scrXX, x2);    
    y2 = std::min(_scrYY, y2);
    
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;
    
    int copyBytes = sizeX << _bppShift;
    int skip = _width << _bppShift; 
    
    uint8_t* sour = (_colBit == 16 ? (uint8_t*)_buf16 : _buf8) + (backToFront ? _backBuff : _frontBuff);
    uint8_t* dest = (_colBit == 16 ? (uint8_t*)_buf16 : _buf8) + (backToFront ? _frontBuff : _backBuff);
    sour += (*(_fastY + y1) + x1) << _bppShift;  
    dest += (*(_fastY + y) + x) << _bppShift;  
    
    while (sizeY-- > 0){
        memcpy(dest, sour, copyBytes);
        dest += skip;
        sour += skip;
    }
}

void VGA_esp32s3::blit(int dx, int dy, int sx1, int sy1, int sx2, int sy2, bool backToFront) {
    if (!_dBuff) return;

    // нормализация координат источника
    if (sx1 > sx2) std::swap(sx1, sx2);
    if (sy1 > sy2) std::swap(sy1, sy2);
    if (sx1 > _scrXX || sy1 > _scrYY || sx2 < 0 || sy2 < 0) return;

    // обрезка источника по экрану
    sx1 = std::max(0, sx1);
    sy1 = std::max(0, sy1);
    sx2 = std::min(_scrXX, sx2);
    sy2 = std::min(_scrYY, sy2);

    int width  = sx2 - sx1 + 1;
    int height = sy2 - sy1 + 1;
    if (width <= 0 || height <= 0) return;

    // указатели на буферы
    uint8_t* sour = (_colBit == 16 ? (uint8_t*)_buf16 : _buf8) + (backToFront ? _backBuff : _frontBuff);
    uint8_t* dest = (_colBit == 16 ? (uint8_t*)_buf16 : _buf8) + (backToFront ? _frontBuff : _backBuff);

    // смещение источника и приёмника
    sour += (*(_fastY + sy1) + sx1) << _bppShift;
    dest += (*(_fastY + dy ) + dx ) << _bppShift;

    int copyBytes = width << _bppShift;
    int lineSize  = _width << _bppShift;

    // если области пересекаются в одном буфере — надо использовать memmove
    bool overlap = (backToFront ? false : true) && 
                   (abs(dy - sy1) < height && abs(dx - sx1) < width);

    if (overlap) {
        // копируем снизу вверх при перекрытии
        sour += (height - 1) * lineSize;
        dest += (height - 1) * lineSize;
        while (height-- > 0) {
            memmove(dest, sour, copyBytes);
            sour -= lineSize;
            dest -= lineSize;
        }
    } else {
        // обычное копирование сверху вниз
        while (height-- > 0) {
            memcpy(dest, sour, copyBytes);
            sour += lineSize;
            dest += lineSize;
        }
    }
}
