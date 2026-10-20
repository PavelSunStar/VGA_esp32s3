#include "../VGA_esp32.h"
#include <esp_LCD_panel_ops.h>
#include "esp_heap_caps.h"

void VGA_esp32::destroy(){
    if (_s.fb0) free(_s.fb0);
    if (_s.fb1) free(_s.fb1);
    if (_s.fLine8) free(_s.fLine8);
    if (_s.bLine8) free(_s.bLine8);
    if (_s.fLine16) free(_s.fLine16);
    if (_s.bLine16) free(_s.bLine16);
    if (_s.pal) free(_s.pal);
    if (_s.tmpBuf) free(_s.tmpBuf);

    _s = {};
}

bool VGA_esp32::setMode(ScreenMode m){
    const char* modeName;

    switch (m){
        case Mode640x480:
            _m = MODE640x480_60Hz;
            _s.scale = 0;
            modeName = "640x480";
            break;

        case Mode320x240:
            _m = MODE640x480_60Hz;
            _s.scale = 1;
            modeName = "320x240";
            break;

        case Mode160x120:
            _m = MODE640x480_60Hz;
            _s.scale = 2;
            modeName = "160x120";
            break;

        default:
            Serial.printf("[VGA] Unsupported mode: %d\n", (int)m);
            return false;
    }

    Serial.printf("[VGA] Mode: %s\n", modeName);

    return true;
}

void VGA_esp32::setScreenDimentions(){
    _s.width    = _m.hRes >> _s.scale;
    _s.height   = _m.vRes >> _s.scale;
    _s.maxX     = _s.width - 1;
    _s.maxY     = _s.height - 1;
    _s.cx       = _s.width >> 1;
    _s.cy       = _s.height >> 1;
    _s.lineSize = _s.width * ((_s.bpp == ColorDepth::BPP_8 || _s.usePal) ? 1 : 2);
    _s.size     = _s.width * _s.height;
    _s.fullSize = _s.lineSize * _s.height;

    //Viewport;
    _s.x0 = 0; 
    _s.y0 = 0;
    _s.x1 = _s.maxX;
    _s.y1 = _s.maxY;
}

bool VGA_esp32::getMemory(){
    if (!PSRAM_OK) {
        Serial.println("\nPlease turn on [PSRAM]...");
        return false;
    }

    int lines = _s.height;
    bool is8bit = (_s.bpp == ColorDepth::BPP_8 || _s.usePal);

    // Front buffer
    if (!(_s.fb0 = allocateMemory(_s.fullSize, true))) return false;
    memset(_s.fb0, 0, _s.fullSize);

    if (is8bit) {
        _s.fLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
        if (!_s.fLine8) return false;
    } else {
        _s.fLine16 = (uint16_t**)malloc(sizeof(uint16_t*) * lines);
        if (!_s.fLine16) return false;
    }

    // Back buffer
    if (_s.dBuff){
        if (!(_s.fb1 = allocateMemory(_s.fullSize, true))) return false;
        memset(_s.fb1, 0, _s.fullSize);

        if (is8bit) {            
            _s.bLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
            if (!_s.bLine8) return false;
        } else {
            _s.bLine16 = (uint16_t**)malloc(sizeof(uint16_t*) * lines);
            if (!_s.bLine16) return false;
        }
    } else {
        _s.fb1 = _s.fb0;
    }

    // Palette
    if (_s.usePal){
        size_t size = (256 << _s.scale) * sizeof(uint16_t);                
        if (!(_s.pal = (uint16_t*)malloc(size))) return false;
        Serial.println(size);
        loadPalDef();
    }

    _s.tmpBuf = malloc(_s.lineSize << 2);
    if (!_s.tmpBuf) return false;

    return true;
}

void VGA_esp32::setBufferAddr(){
    bool is8bit = (_s.bpp == ColorDepth::BPP_8 || _s.usePal);

    // FRONT
    if (is8bit){
        uint8_t* src = (uint8_t*)_s.fb0;

        for (int y = 0; y < _s.height; y++){
            _s.fLine8[y] = src;
            src += _s.width;
        }
    } else {
        uint16_t* src = (uint16_t*)_s.fb0;

        for (int y = 0; y < _s.height; y++){
            _s.fLine16[y] = src;
            src += _s.width;
        }
    }

    // BACK
    if (_s.dBuff){
        if (is8bit)        {
            uint8_t* src = (uint8_t*)_s.fb1;

            for (int y = 0; y < _s.height; y++){
                _s.bLine8[y] = src;
                src += _s.width;
            }
        } else {
            uint16_t* src = (uint16_t*)_s.fb1;

            for (int y = 0; y < _s.height; y++){
                _s.bLine16[y] = src;
                src += _s.width;
            }
        }
    } else {
        _s.bLine8  = _s.fLine8;
        _s.bLine16 = _s.fLine16;
    }
}

int VGA_esp32::optimal_bounce_buffer_px(){
    int res = 0;

    if (_m.hRes == 640 && _m.vRes == 480) {
        res = 30720 >> ((_s.usePal || _s.bpp == ColorDepth::BPP_16) ? 1 : 0);
    }
   
    _sp.lastPos     = _m.hRes * _m.vRes - res;
    _sp.shift       = (_s.bpp == ColorDepth::BPP_16 ? 1 : 0);
    _sp.lines       = (res / _m.hRes) >> _s.scale;
    _sp.pixels      = _m.hRes >> 3;
    _sp.copyBytes   = _m.hRes << _sp.shift;
    _sp.copyBytes2x = _sp.copyBytes << 1; 
    _sp.copyBytes4x = _sp.copyBytes2x << 1;
    _sp.skip        = (_m.hRes >> 2) << _sp.shift;

    return res;
}

bool VGA_esp32::setRGBPanel(){
    panel_config = {};
    auto& p = panel_config;

    //Timing
    p.clk_src                       = LCD_CLK_SRC_DEFAULT;
    p.timings.pclk_hz               = _m.pclk_hz;
    p.timings.h_res                 = _m.hRes;
    p.timings.v_res                 = _m.vRes;

    p.timings.hsync_pulse_width     = _m.hSync;
    p.timings.hsync_back_porch      = _m.hBack;
    p.timings.hsync_front_porch     = _m.hFront;
    p.timings.flags.hsync_idle_low  = (_m.hPol == 1) ^ 1;

    p.timings.vsync_pulse_width     = _m.vSync;
    p.timings.vsync_back_porch      = _m.vBack;
    p.timings.vsync_front_porch     = _m.vFront;
    p.timings.flags.vsync_idle_low  = (_m.vPol == 1) ^ 1;
    
    p.timings.flags.de_idle_high    = true;
    p.timings.flags.pclk_active_neg = true;
    p.timings.flags.pclk_idle_high  = true;

    //Panel config
    p.data_width             = (_s.usePal || _s.bpp == ColorDepth::BPP_16) ? 16 : 8;        
    p.bits_per_pixel         = p.data_width;   
    p.num_fbs                = 0;

    p.bounce_buffer_size_px  = optimal_bounce_buffer_px();
    p.sram_trans_align       = 4;
    p.psram_trans_align      = 32;

    //Pins config
    if (_s.usePal || _s.bpp == ColorDepth::BPP_16){
        // B0..B4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[i] = _p.b[i];

        // G0..G5
        for (int i = 0; i < 6; i++)
            panel_config.data_gpio_nums[5 + i] = _p.g[i];

        // R0..R4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[11 + i] = _p.r[i];
    } else {
        panel_config.data_gpio_nums[0] = _p.b[3];
        panel_config.data_gpio_nums[1] = _p.b[4];
        panel_config.data_gpio_nums[2] = _p.g[3];
        panel_config.data_gpio_nums[3] = _p.g[4];
        panel_config.data_gpio_nums[4] = _p.g[5];
        panel_config.data_gpio_nums[5] = _p.r[2];
        panel_config.data_gpio_nums[6] = _p.r[3];
        panel_config.data_gpio_nums[7] = _p.r[4];
    }
    panel_config.hsync_gpio_num = _p.h;
    panel_config.vsync_gpio_num = _p.v;
    panel_config.de_gpio_num = -1;
    panel_config.pclk_gpio_num = -1; //??
    panel_config.disp_gpio_num = -1;

    //Flags
    panel_config.flags.disp_active_low = true;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = false;
    panel_config.flags.double_fb = false;
    panel_config.flags.no_fb = true;
    panel_config.flags.bb_invalidate_cache = false;
    
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_lcd_new_rgb_panel error: %d\n", err);
        return false;
    }

    Serial.println("RGB Panel set...Ok");
    return true;
}

void VGA_esp32::regCallBack(){
    esp_lcd_rgb_panel_event_callbacks_t cb = {
        .on_color_trans_done = nullptr,
        .on_vsync            = on_vsync,
        .on_bounce_empty = (_s.usePal ? on_bounce_empty_pal : on_bounce_empty),           
        .on_frame_buf_complete = nullptr
    };

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cb, this)); 
    Serial.println("Registered callBacks...Ok"); 
}

bool VGA_esp32::initPanel(){
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(err));
        return false;
    }

    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("Init panel complete...Ok");
    return true;
}

bool VGA_esp32::regSemaphore(){
    if (_s.dBuff){
        _sem_vsync_end = xSemaphoreCreateBinary();
        _sem_gui_ready = xSemaphoreCreateBinary();

        if (!_sem_vsync_end || !_sem_gui_ready){
            Serial.println("Error: Semaphores init fail.");
            return (_semInited = false);
        }
    } else {
        _sem_vsync_end = nullptr;
        _sem_gui_ready = nullptr;

        Serial.println("Double buffer not selected.");
        return (_semInited = false);
    }

    Serial.println("Registered semaphores...Ok");
    return (_semInited = true);
}

void VGA_esp32::loadPalDef(){
    uint16_t* pal = _s.pal;

    if (_s.scale == 0){
        for (int i = 0; i < 256; i++){
            *pal++ = TABLE_332to565[i];
        }
    } else if (_s.scale == 1){
        for (int i = 0; i < 256; i++){
            uint16_t col = TABLE_332to565[i];
            *pal++ = col;
            *pal++ = col;
        }
    } else if (_s.scale == 2){
        for (int i = 0; i < 256; i++){
            uint16_t col = TABLE_332to565[i];
            *pal++ = col;
            *pal++ = col;
            *pal++ = col;
            *pal++ = col;
        }
    }
}