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
        _scrWidth, _scrHeight, _bpp,
        _scale); 
    Serial.printf("Viewport: (%d, %d - %d, %d)\n", _vX1, _vY1, _vX2, _vY2);

    Serial.printf("Buffer address: %p, Buffer size: %d\n", ((_bpp == 16) ? (void*)_buf16 : (void*)_buf8), ((_dBuff) ? _scrSize << 1 : _scrSize)); 
    Serial.printf("Screen size: %d, Block size: %d\n", _scrSize, _bounce_buffer_size_px);
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
    //uint32_t caps = MALLOC_CAP_DMA | (_psRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_INTERNAL) | MALLOC_CAP_8BIT;
    uint32_t caps = MALLOC_CAP_SPIRAM;
    if (_bpp == 16){
        _buf16 = (uint16_t*)heap_caps_malloc((_dBuff ? _scrSize << 1 : _scrSize) , caps);
        assert(_buf16);
        memset(_buf16, 0, (_dBuff ? _scrSize << 1 : _scrSize));
    } else {
        _buf8 = (uint8_t*)heap_caps_malloc((_dBuff ? _scrSize << 1 : _scrSize) , caps);
        assert(_buf8);
        memset(_buf8, 0, (_dBuff ? _scrSize << 1 : _scrSize));
    }
    
    regCallbackSemaphore();
    //esp_lcd_panel_disp_on_off(_panel_handle, true);

    printInfo();
    Serial.println("Init...Ok");

    _fastY = (int*) malloc(_scrHeight * sizeof(int));
    for (int y = 0; y < _scrHeight; y++)
        _fastY[y] = _scrWidth * y;

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

    if (vga->getBpp() == 16){
        uint16_t* dest = (uint16_t*)bounce_buf;
        uint16_t* sour = vga->_buf16 + vga->_frontBuff;

        if (vga->getScale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes << 1);
        } else if (vga->getScale() == 1){
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
        uint8_t* sour = vga->_buf8 + vga->_frontBuff;

        if (vga->getScale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->getScale() == 1){
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
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;

    return false; 
}

bool IRAM_ATTR VGA_esp32s3::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
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
    _psRam = psRam;
    _dBuff = dBuff;

    _xx = _width - 1;
    _yy = _height - 1;    
    _size = _width * _height;
    _lastBounceBufferPos = _size - _bounce_buffer_size_px;

    _scrWidth = _width >> _scale;
    _scrHeight = _height >> _scale;
    _scrXX = _scrWidth - 1;
    _scrYY = _scrHeight - 1;
    _scrSize = _scrWidth * _scrHeight * ((_colBit == 16) ? sizeof(uint16_t) : sizeof(uint8_t));

    _frontBuff = 0;
    _backBuff = (_dBuff) ? _scrSize : 0;
    _tik = _width >> 4;
    _lines = (_bounce_buffer_size_px / _width) >> _scale;
    _width2X = _width << 1;
    _width4X = _width2X << 1;
    setViewport(0, 0, _scrXX, _scrYY);

    if (!setPanelConfig()) return false;

    return true;
}

void VGA_esp32s3::setViewport(int x1, int y1, int x2, int y2) {
    if (x1 == x2 || y1 == y2) return;

    // Обеспечение x1 <= x2 и y1 <= y2
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Проверка на нулевую или некорректную область
    if (x1 < 0 || y1 < 0 || x2 >= _scrWidth || y2 >= _scrHeight) return;

    // Вычисление ширины и высоты области просмотра
    _vX1 = x1;
    _vY1 = y1;
    _vX2 = x2;
    _vY2 = y2;
    _vWidth = _vX2 - _vX1 + 1;
    _vHeight = _vY2 - _vY1 + 4;
} 

void VGA_esp32s3::swap() {
    if (_dBuff){
        xSemaphoreGive(_sem_gui_ready);
        xSemaphoreTake(_sem_vsync_end, portMAX_DELAY);
    }
}


/*
bool IRAM_ATTR VGA_esp32s3::on_vsync(esp_VGA_panel_handle_t panel,
                                     const esp_VGA_rgb_panel_event_data_t *edata,
                                     void *user_ctx)
{
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;
    BaseType_t high_task_awoken = pdFALSE;

    // Wait until LVGL has finished 
    if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, &high_task_awoken) == pdTRUE) {
        // Indicate that the VSYNC event has ended, and it's safe to proceed with flushing the buffer.
        xSemaphoreGiveFromISR(vga->_sem_vsync_end, &high_task_awoken);
    }

    vga->_bufIndex++;
    return high_task_awoken == pdTRUE;
}

void VGA_esp32s3::swap() {
    // LVGL has finished
    xSemaphoreGive(_sem_gui_ready);
    // Now wait for the VSYNC event. 
    xSemaphoreTake(_sem_vsync_end, portMAX_DELAY);

    // pass the draw buffer to the driver
    esp_VGA_panel_draw_bitmap(_panel_handle, 0, 0, 639, 479, _buf8Auto[1]);
}

//**********************************************************************
    // Дополнительные параметры
panel_config.data_width = 8;
panel_config.bits_per_pixel = _bpp;
panel_config.flags.fb_in_psram = _psRam;
//panel_config.sram_trans_align = 0;
//panel_config.psram_trans_align = 64;
//panel_config.bounce_buffer_size_px = _bounce_buffer_size_px;

// мы сами делаем malloc → запрещаем драйверу создавать буферы
panel_config.num_fbs = 2;
panel_config.flags.double_fb = true;
panel_config.flags.no_fb = false;
//panel_config.flags.refresh_on_demand = true;

// Создание RGB-панели
    esp_err_t ret = esp_VGA_new_rgb_panel(&panel_config, &_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to create RGB panel: %s\n", esp_err_to_name(ret));
        return false;
    }

    // Сброс и инициализация
    ret = esp_VGA_panel_reset(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(ret));
        return false;
    }
    ret = esp_VGA_panel_init(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(ret));
        return false;
    }

    // Выделение буферов ДО draw_bitmap
    _buf8Auto[0] = (uint8_t*)heap_caps_malloc(_width * _height * (_bpp / 8), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    _buf8Auto[1] = (uint8_t*)heap_caps_malloc(_width * _height * (_bpp / 8), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    if (!_buf8Auto[0] || !_buf8Auto[1]) {
        Serial.println("ERROR: Failed to alloc buffers");
        return false;
    }

    // Тест: заполните буфер цветом (например, синий для RGB565)
    memset(_buf8Auto[0], 0xFF, _width * _height * (_bpp / 8));  // Белый/полный для теста

    // Теперь draw_bitmap после выделения
    ret = esp_VGA_panel_draw_bitmap(_panel_handle, 0, 0, _width, _height, _buf8Auto[0]);  // Полный экран для теста
    if (ret != ESP_OK) {
        Serial.printf("ERROR: draw_bitmap failed: %s\n", esp_err_to_name(ret));
        return false;
    }

    // В false-режиме refresh не нужен, но для теста вызовите
    esp_VGA_rgb_panel_refresh(_panel_handle);
    //******************************************************************************
    // 
    // 
    // 
    // bool IRAM_ATTR VGA_esp32s3::on_bounce_empty(esp_VGA_panel_handle_t panel,
                                            void *bounce_buf,
                                            int pos_px,
                                            int len_bytes,
                                            void *user_ctx)
{
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;
    uint8_t* dest = (uint8_t*)bounce_buf;

    // копируем из текущего фронт-буфера
    uint8_t* sour = vga->_buf8 + vga->_frontBuff + pos_px;
    //memcpy(dest, sour, len_bytes);
    // Быстрая копия (ROM memcpy если доступно, иначе обычный memcpy)
    fast_memcpy(dest, sour, len_bytes);


    // если дошли до конца кадра → можно разрешить swap
    if (pos_px >= vga->_lastBounceBufferPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, NULL) == pdTRUE) {
            // передаём драйверу новый буфер 
            if (vga->_colBit == 16){
                std::swap(vga->_buf16[0], vga->_buf16[1]);
            } else {
                std::swap(vga->_buf8[0], vga->_buf8[1]);
            }
            std::swap(vga->_frontBuff, vga->_backBuff);

            xSemaphoreGiveFromISR(vga->_sem_vsync_end, NULL);
        }
    }

    return true;
}
    // 
    //     if (vga->_bpp == 16){
        if (vga->_scale == 2){

        } else if (vga->_scale == 1){

        } else {
            
        }
    } else if (vga->_bpp == 8){
        uint8_t* dest = (uint8_t*)bounce_buf;

        if (vga->_scale == 2){
            uint8_t* sour = vga->_buf8 + vga->_frontBuff + (pos_px >> 4);
            uint8_t* savePos = dest;
            int lines = (vga->_bounce_buffer_size_px / vga->_width) >> vga->_scale;
            
            while (lines-- > 0){
                int tik = vga->_width / 16;

                while (tik-- > 0){
                    *dest++ = *sour; *dest++ = *sour; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour; *dest++ = *sour; *dest++ = *sour++; 
                }
                memcpy(dest, savePos, vga->_width); dest += vga->_width;
                memcpy(dest, savePos, vga->_width << 1); dest += vga->_width << 1;
            }
        } else if (vga->_scale == 1){
            uint8_t* sour = vga->_buf8 + vga->_frontBuff + (pos_px >> 2);
            int lines = 1;//(len_bytes / vga->_width) / 2;
            
            while (lines-- > 0){
                uint8_t* savePos = dest;
                int tik = vga->_width / 16;

                while (tik-- > 0){
                    *dest++ = *sour; *dest++ = *sour++; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour++; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour++; *dest++ = *sour; *dest++ = *sour++; 
                    *dest++ = *sour; *dest++ = *sour++; *dest++ = *sour; *dest++ = *sour++; 
                }
                memcpy(dest, savePos, vga->_width); 
                dest += vga->_width;
            }
        } else {
            uint8_t* sour = vga->_buf8[0] + pos_px;
            memcpy(dest, sour, len_bytes);
        }
    }

            if (_dBuff){
            _buf8[0] = (uint8_t*)heap_caps_malloc(_scrSize, caps); 
            assert(_buf8[0]);
            memset(_buf8[0], 0, _scrSize);

            _buf8[1] = (uint8_t*)heap_caps_malloc(_scrSize, caps); 
            assert(_buf8[1]);
            memset(_buf8[1], 0, _scrSize);
        } else {
            _buf8[0] = (uint8_t*)heap_caps_malloc(_scrSize, caps); 
            assert(_buf8[0]);
            memset(_buf8[0], 0, _scrSize);
        } 
    //  
//CallBack's*****************************************************
bool IRAM_ATTR VGA_esp32s3::on_bounce_empty(
    esp_VGA_panel_handle_t panel,
    void *bounce_buf,
    int pos_px,
    int len_bytes,
    void *user_ctx){
    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;

    if (vga->_bpp == 16){
        uint16_t* dest = (uint16_t*)bounce_buf;

        if (vga->_scale == 0){
            uint16_t* sour = vga->_buf16 + vga->_frontBuff + pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->_scale == 1) {

        } else {

        }
    } else {
        uint8_t* dest = (uint8_t*)bounce_buf;

        if (vga->_scale == 0){
            uint8_t* sour = vga->_buf8 + vga->_frontBuff + pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->_scale == 1){
            uint8_t* sour = vga->_buf8 + vga->_frontBuff + (pos_px >> 2);
            int lines = vga->_lines;

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
            uint8_t* sour = vga->_buf8 + vga->_frontBuff + (pos_px >> 4);
            int lines = vga->_lines;

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
    if (pos_px >= vga->_lastBounceBufferPos && vga->_dBuff && vga->_swap) {
        if (xSemaphoreTakeFromISR(vga->_sem_gui_ready, NULL) == pdTRUE) {
            std::swap(vga->_frontBuff, vga->_backBuff);
            xSemaphoreGiveFromISR(vga->_sem_vsync_end, NULL);
        }
    }

    return true;
}

bool IRAM_ATTR VGA_esp32s3::on_bounce_empty(
    esp_VGA_panel_handle_t panel,
    void *bounce_buf,
    int pos_px,
    int len_bytes,
    void *user_ctx){

    VGA_esp32s3* vga = (VGA_esp32s3*)user_ctx;
    int lines = vga->_lines;

    if (vga->getBpp() == 16){
        uint16_t* dest = (uint16_t*)bounce_buf;
        uint16_t* sour = vga->_buf16 + vga->_frontBuff;

        if (vga->getScale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->getScale() == 1){
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
                auto savePos = dest;

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
        uint8_t* sour = vga->_buf8 + vga->_frontBuff;

        if (vga->getScale() == 0){
            sour += pos_px;
            memcpy(dest, sour, len_bytes);
        } else if (vga->getScale() == 1){
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
    // */
