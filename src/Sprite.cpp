#include "Sprite.h"

Sprite::Sprite(VGA_esp32 &vga) : _vga(vga){

}

Sprite::~Sprite(){
    destroy(); 
}

void Sprite::destroy(){
    if (_buf)   free(_buf);    
    if (_img)       delete[] _img;
    if (_line8)     delete[] _line8; 
    if (_line16)    delete[] _line16; 

    _buf    = nullptr;
    _line8  = nullptr;
    _line16 = nullptr;
    _img    = nullptr;

    _images  = 0;
    _created = false;
}

int Sprite::setImageDimentions(uint16_t image, int width, int height){
    auto& s = _vga._s;
    auto& im = _img[image];

    im.width     = width;
    im.height    = height;
    im.maxX      = im.width - 1;
    im.maxY      = im.height - 1;
    im.cx        = im.width >> 1;
    im.cy        = im.height >> 1;
    im.lineSize  = im.width << (s.bpp == ColorDepth::BPP_16 ? 1 : 0);
    im.size      = im.width * im.height;
    im.fullSize  = im.lineSize * im.height;

    return im.height;
}

bool Sprite::setBufferAddr(int lines){
    int offset;

    if (_vga._s.bpp == ColorDepth::BPP_8){
        _line8 = new uint8_t*[lines];
        if (!_line8) return false;

        uint8_t* buf = (uint8_t*)_buf;        
        for (int i = 0; i < _images; i++){
            auto& s = _img[i];
            offset = s.offsetLine;

            for (int j = 0; j < s.height; j++){
                _line8[j + offset] = buf;
                buf += s.width;
            }
        }
    } else {
        _line16 = new uint16_t*[lines];
        if (!_line16) return false;

        uint16_t* buf = (uint16_t*)_buf;
        for (int i = 0; i < _images; i++){
            auto& s = _img[i];
            offset = s.offsetLine;

            for (int j = 0; j < s.height; j++){
                _line16[j + offset] = buf;
                buf += s.width;
            }
        }        
    }

    return true;
}

bool Sprite::create(int width, int height, uint16_t images, bool psRam){
    destroy();
    if (width <= 1 || height <= 1 || images < 1) return (_created = false);

    _images = images;      
    _img = new Image[images];

    int lines = 0;     
    int offset = 0;
    size_t allSize = 0;

    for (int i = 0; i < _images; i++){
        lines += setImageDimentions(i, width, height);
        _img[i].offsetLine = offset;
        allSize += _img[i].fullSize;
        offset += _img[i].height;
    }

    _psRam = psRam;
    if (!(_buf = _vga.allocateMemory(allSize, _psRam))){
        destroy(); 
        return false;
    }

    if (!setBufferAddr(lines)){
        destroy();
        return false;
    } 

    return (_created = true);
}

void Sprite::image(int x, int y, uint16_t frame, bool getFrame){
    auto& s = _vga._s;    
    if (!s.inited || !_created || frame >= _images) return;
    if (x > s.x1 || y > s.y1) return;  

    Image& im = _img[frame];
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;
        uint8_t* img = (s.bpp == BPP_16) ? (uint8_t*)&_line16[im.offsetLine][0] : &_line8[im.offsetLine][0];
        uint8_t* scr = (s.bpp == BPP_16) ? (uint8_t*)&s.bLine16[y][x] : &s.bLine8[y][x];

        const int copyBytes = im.lineSize;
        const int dstSkip = s.lineSize;

        if (getFrame){
            while (lines--){
                memcpy(img, scr, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        } else {
            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        }
    } else { 
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

        int copyX = (im.width - sxl - sxr) << s.shift;
        int copyY =  im.height - syu - syd;
        if (copyX <= 0 || copyY <= 0) return;
 
        int ioy = im.offsetLine + syu;
        uint8_t* img = (s.bpp == BPP_16) ? (uint8_t*)&_line16[ioy][sxl] : &_line8[ioy][sxl];

        int soy = y + syu; int sox = x + sxl;
        uint8_t* scr = (s.bpp == BPP_16) ? (uint8_t*)&s.bLine16[soy][sox] : &s.bLine8[soy][sox];

        const int imgStep = im.lineSize;
        const int scrStep = s.lineSize;

        if (getFrame){
            while (copyY--){
                memcpy(img, scr, copyX);
                img += imgStep;
                scr += scrStep;
            }  
        } else {
            while (copyY--){
                memcpy(scr, img, copyX);
                img += imgStep;
                scr += scrStep;
            } 
        }  
    }
}

void Sprite::putSprite(int x, int y, uint16_t maskColor, uint16_t frame){
    auto& s = _vga._s;    
    if (!s.inited || !_created || frame >= _images) return;
    if (x > s.x1 || y > s.y1) return;

    Image &im = _img[frame];
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;  
        int skip = s.width - im.width;  

        if (s.usePal || s.bpp == BPP_8){
            uint8_t* img = &_line8[im.offsetLine][0];
            uint8_t* scr = &s.bLine8[y][x]; 

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += skip;
            } 
        } else {
            uint16_t* img = &_line16[im.offsetLine][0];
            uint16_t* scr = &s.bLine16[y][x];

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += skip;
            } 
        }
    } else { 
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

        int copyX = im.width  - sxl - sxr;
        int copyY =  im.height - syu - syd; 
        if (copyX <= 0 || copyY <= 0) return;   
        int skipSour = im.width - copyX;
        int skipScr = s.width - copyX;

        if (s.usePal || s.bpp == BPP_8){
            uint8_t* img = &_line8[im.offsetLine + syu][sxl];
            uint8_t* scr = &s.bLine8[y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            }   
        } else {
            uint16_t* img = &_line16[im.offsetLine + syu][sxl];
            uint16_t* scr = &s.bLine16[y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            } 
        }
    }
}

void Sprite::putAffineSprite(int dstX, int dstY, float ang, int zoomX, int zoomY, uint16_t maskColor, uint8_t frame){
    auto& s = _vga._s; 
    if (!s.inited || !_created || frame >= _images) return;
 
    Image& im = _img[frame];
    const int sourW = im.width;
    const int sourH = im.height;    

    _vga._mat = Matrix::make((float)dstX, (float)dstY, 
                (sourW - 1) * 0.5f, (sourH - 1) * 0.5f, 
                angle(ang), 
                percentTo(zoomX), percentTo(zoomY)
    );
    
    _vga._rectMat = Matrix::bounds(_vga._mat, (float)sourW, (float)sourH);
    int x0 = (int)floorf(_vga._rectMat.sx); // округляет число вниз до ближайшего целого
    int y0 = (int)floorf(_vga._rectMat.sy);
    int x1 = (int)ceilf(_vga._rectMat.ex);  // округляет число вверх до ближайшего целого.
    int y1 = (int)ceilf(_vga._rectMat.ey);  

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);
    if (x0 >= x1 || y0 >= y1) return;

    if (!Matrix::invert(_vga._invMat, _vga._mat)) return; 
    int row_u = _vga._invMat.a * x0 + _vga._invMat.b * y0 + _vga._invMat.c;
    int row_v = _vga._invMat.d * x0 + _vga._invMat.e * y0 + _vga._invMat.f;    

    int u, v, sx, sy;
    if (s.usePal || s.bpp == BPP_8){
        uint8_t** srcLines = _line8 + im.offsetLine;
        uint8_t* dstBase   = &s.bLine8[y0][x0];
        const uint8_t mask = (uint8_t)maskColor;

        while (y0++ < y1){
            uint8_t* dst = dstBase;
            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x0; x < x1; x++){
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW &&
                    (unsigned)sy < (unsigned)sourH)
                {
                    uint8_t* srcRow = srcLines[sy];
                    uint8_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += _vga._invMat.a;
                v += _vga._invMat.d;
            }

            dstBase += s.width;
            row_u   += _vga._invMat.b;
            row_v   += _vga._invMat.e;
        }  
    } else {
        uint16_t** sour = _line16 + im.offsetLine;
        uint16_t* dest = &s.bLine16[y0][x0];

        while (y0++ < y1){
            uint16_t* dst = dest;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = u >> FP_SHIFT;
                sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW && (unsigned)sy < (unsigned)sourH){
                    uint16_t col = sour[sy][sx];
                    if (col != maskColor) *dst = col;
                }

                dst++;
                u += _vga._invMat.a;
                v += _vga._invMat.d;                
            }

            dest += s.width;
            row_u   += _vga._invMat.b;
            row_v   += _vga._invMat.e;            
        }
    }
}    

/*
// Draw preconverted RGB332 or RGB565 sprites directly to the framebuffer.
void Sprite::putBankSprite(int x, int y, const BankSprite& sp, int maskColor) {
    auto& s = _vga._s;
    if (!s.inited || !sp.pixels || !sp.width || !sp.height || s.usePal) return;
    const bool mode8 = (s.bpp == ColorDepth::BPP_8);
    const bool mode16 = (s.bpp == ColorDepth::BPP_16);
    if ((!mode8 && !mode16) || (mode8 && sp.format != 3) ||
        (mode16 && sp.format != 7)) return;
    const int w = sp.width, h = sp.height;
    const size_t bpp = mode16 ? 2u : 1u;
    if ((size_t)w * h > SIZE_MAX / bpp || sp.size < (size_t)w * h * bpp) return;
    const int64_t right = (int64_t)x + w, bottom = (int64_t)y + h;
    if (right <= s.x0 || bottom <= s.y0 || x > s.x1 || y > s.y1) return;
    const int dx0 = x < s.x0 ? s.x0 : x;
    const int dy0 = y < s.y0 ? s.y0 : y;
    const int dx1 = right > (int64_t)s.x1 + 1 ? s.x1 + 1 : (int)right;
    const int dy1 = bottom > (int64_t)s.y1 + 1 ? s.y1 + 1 : (int)bottom;
    if (dx0 >= dx1 || dy0 >= dy1) return;
    const int sx0 = dx0 - x, sy0 = dy0 - y;
    const int copyW = dx1 - dx0, copyH = dy1 - dy0;
    const bool masked = (maskColor >= 0 && maskColor <= (mode8 ? 255 : 65535));

    if (mode8) {
        if (!s.bLine8) return;
        for (int row = 0; row < copyH; ++row) {
            const uint8_t* src = sp.pixels + (size_t)(sy0 + row) * w + sx0;
            uint8_t* dst = &s.bLine8[dy0 + row][dx0];
            if (!masked) memcpy(dst, src, copyW);
            else for (int i = 0; i < copyW; ++i)
                if (src[i] != (uint8_t)maskColor) dst[i] = src[i];
        }
    } else {
        if (!s.bLine16) return;
        // The caller supplies a RGB332 color key (0..255), not RGB565.
        const uint16_t mask565 = masked ? RGB332to565((uint8_t)maskColor) : 0;
        for (int row = 0; row < copyH; ++row) {
            const uint16_t* src = (const uint16_t*)sp.pixels + (size_t)(sy0 + row) * w + sx0;
            uint16_t* dst = &s.bLine16[dy0 + row][dx0];
            if (!masked) memcpy(dst, src, (size_t)copyW * sizeof(uint16_t));
            else for (int i = 0; i < copyW; ++i)
                if (src[i] != mask565) dst[i] = src[i];
        }
    }
}
    */
