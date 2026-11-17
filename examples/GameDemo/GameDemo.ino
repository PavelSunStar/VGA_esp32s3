#include <VGA_esp32.h>
#include <Sprite.h>
#include <math.h>
#include <string.h>
#include <new>
#include "GameTypes.h"
#define RACER_LIBRARY_SPANS 1 // 1: compare library hLineLen against direct memset
#include "sunset.h"
#include "car.h"
#include "palm.h"
// VGA RACER - gamePal_9_DualCore_Fix: live palette experiment
// ESP32-S3 / COL332 / 320x240
VGA_esp32 vga;
Sprite carSprite(vga);
Sprite palmSprite(vga);
Sprite cityPanorama(vga);
bool cityReady=false;
// SCREEN
constexpr int W = 320;
constexpr int H = 240;
constexpr int BG_W = 768;
constexpr int BG_H = sizeof(_sunset) / BG_W;
constexpr int CAR_W = 72;
constexpr int CAR_H = 48;
// ROAD
constexpr int ROAD_WIDTH = 180;
constexpr int BASE_HORIZON = 120;
constexpr int MAX_CURVE = 110;
constexpr int MAX_HILL = 35;
constexpr int VIEW_DISTANCE = 4096;
constexpr int TREE_SPACING = 128;
constexpr int MAX_DEPTH = 155;
constexpr int MIN_TREE_DEPTH = 5;
// COL332
constexpr uint8_t COL332( uint8_t r, uint8_t g, uint8_t b){
    return (r & 0xE0) |
           ((g & 0xE0) >> 3) |
           (b >> 6);
}
constexpr uint8_t BLACK = 0x00;
constexpr uint8_t WHITE = 0xFF;
constexpr uint8_t RED   = 0xE0;
constexpr uint8_t GRASS1 = 0x14;
constexpr uint8_t GRASS2 = 0x18;
constexpr uint8_t ROAD1 = 0x49;
constexpr uint8_t ROAD2 = 0x6D;
// COAST COLORS
constexpr uint8_t SEA1 = 0x0F;
constexpr uint8_t SEA2 = 0x13;
constexpr uint8_t SEA3 = 0x17;
constexpr uint8_t SAND1 = 0xF5;
constexpr uint8_t SAND2 = 0xD1;
constexpr uint8_t FOAM = 0xBF;
constexpr uint8_t WAVE = 0x7B;
constexpr uint8_t WET_SAND = 0xAD;
// MOUNTAIN COLORS
constexpr uint8_t ROCK1 = COL332(128,96,64);
constexpr uint8_t ROCK2 = COL332(160,128,64);
constexpr uint8_t MOUNTAIN_FAR =
    COL332(96,64,128);
constexpr uint8_t MOUNTAIN_NEAR =
    COL332(64,64,64);
constexpr uint8_t MOUNTAIN_LIGHT =
    COL332(160,128,96);
constexpr uint8_t SNOW =
    COL332(224,224,255);
// TUNNEL COLORS
constexpr uint8_t TUNNEL_WALL1 =
    COL332(96,96,128);
constexpr uint8_t TUNNEL_WALL2 =
    COL332(64,64,64);
constexpr uint8_t TUNNEL_CEILING =
    COL332(32,32,64);
constexpr uint8_t TUNNEL_LIGHT =
    COL332(255,224,128);
constexpr uint8_t TUNNEL_DARK =
    COL332(32,32,32);
// CITY COLORS
constexpr uint8_t NIGHT_SKY =
    COL332(0,0,64);
constexpr uint8_t CITY_GROUND1 =
    COL332(32,32,64);
constexpr uint8_t CITY_GROUND2 =
    COL332(64,32,64);
constexpr uint8_t NEON_PINK =
    COL332(255,32,192);
constexpr uint8_t NEON_BLUE =
    COL332(0,192,255);
constexpr uint8_t WINDOW_YELLOW =
    COL332(255,224,64);
// Dedicated animated indices: unused by the original assets and static colors.
constexpr uint8_t P_SEA0=0x30, P_SEA1=0x31, P_SEA2=0x32, P_WAVE=0x33;
constexpr uint8_t P_FOAM=0x34, P_NEON_PINK=0x35, P_NEON_BLUE=0x36;
constexpr uint8_t P_LAMP=0x37, P_WINDOW0=0x38, P_WINDOW1=0x39;
constexpr uint8_t P_STAR=0x3A, P_ROOF=0x3B;
constexpr uint8_t P_SKY0=0x3C, P_SKY1=0x3D, P_SKY2=0x3E, P_SKY3=0x3F;
constexpr uint32_t DAY_CYCLE_MS=120000;
uint32_t dayStart=0;
uint16_t dayPhase=0;
uint8_t nightAmount=0;
constexpr uint8_t safeBaseIndex(uint8_t c){
    return (c >= 0x30 && c <= 0x3F) ? (uint8_t)(c-0x20) : c;
}
// LOCATIONS
enum class Location : uint8_t {
    COAST = 0,
    MOUNTAINS,
    TUNNEL,
    CITY, DESERT, BRIDGE, FOREST, VOLCANO, NEON
};
Location location = Location::COAST;
constexpr uint32_t LOCATION_TIME = 20000;
uint32_t locationStart = 0;
const char* locationNames[] = {
    "COAST ROAD",
    "MOUNTAIN PASS",
    "TUNNEL",
    "NIGHT CITY", "DESERT", "SEA BRIDGE", "AUTUMN FOREST", "VOLCANO", "NEON TUNNEL"
};
// GAME STATE
uint32_t gameFrame = 0;
int curve = 0;
int hill = 0;
int horizon = BASE_HORIZON;
int speed = 180;
float playerLane = 0.0f;
float targetLane = 0.0f;
uint32_t overtakes = 0;
// Weather: fixed storage, no allocations in the frame loop.
#define RAIN_COUNT 600
#define RAIN_SPLASHES 32
#define ENABLE_RAIN 1
#define ENABLE_LIGHTNING 1
#define ENABLE_WET_ROAD 1
static uint8_t rainAmount=0, wetAmount=0, lightning=0, snowAmount=0, fogAmount=0;
static int windPower=0;
static uint8_t sandAmount=0;
static uint32_t weatherRng=0x12345678;
static uint32_t weatherRand(){weatherRng^=weatherRng<<13; weatherRng^=weatherRng>>17; weatherRng^=weatherRng<<5; return weatherRng;}
// WORLD ALIVE: one bounded event per visit, with a random start and duration.
static uint32_t eventStart=0, eventDuration=0;
static int eventLocation=-1;
static uint8_t eventPower=0;
static bool eventActive=false;
static const char* eventNames[]={"CLOUD BURST","BLIZZARD","BLACKOUT","CITY STORM","SAND STORM","GALE","LEAF WHIRL","ERUPTION","NEON PULSE"};
static void updateWorldEvent(){
    uint32_t t=millis()-locationStart;
    if(eventLocation!=(int)location){
        eventLocation=(int)location;
        eventStart=3000+weatherRand()%6500;
        eventDuration=4000+weatherRand()%3000;
    }
    eventActive=t>=eventStart && t-eventStart<eventDuration;
    eventPower=0;
    if(eventActive){
        uint32_t a=t-eventStart;
        eventPower=(uint8_t)min(255UL,(unsigned long)min(a,eventDuration-a)*255UL/900UL);
    }
}
static void updateWeather(){
    uint32_t t=millis()-locationStart;
    int storm=45+int(eventPower)*210/255;
    int snow=location==Location::MOUNTAINS ? (55+int(eventPower)*200/255):0;
    snowAmount=snow;
    rainAmount=location==Location::TUNNEL?0:storm*(255-snow)/255;
    wetAmount=location==Location::TUNNEL?0:rainAmount;
    fogAmount=location==Location::MOUNTAINS?snow*3/4:rainAmount/3;
    windPower=int(4*sinf(gameFrame*0.021f)+2*sinf(gameFrame*0.007f));
    sandAmount=location==Location::DESERT?storm:0;
    if(sandAmount){rainAmount=wetAmount=0; fogAmount=sandAmount;}
    if(location==Location::BRIDGE) fogAmount=max(int(fogAmount),storm/2);
    if(location==Location::FOREST || location==Location::VOLCANO || location==Location::NEON){rainAmount=wetAmount=snowAmount=sandAmount=fogAmount=0;}
    if(eventActive && location==Location::BRIDGE) windPower+=eventPower/32;
    lightning=0;
    if(rainAmount>180){uint32_t f=t%6500; if(f<90 || (f>180 && f<250)) lightning=180;}

}
// SPRITE CACHE
CachedSprite palmCache[MAX_DEPTH + 1];
constexpr int TRAFFIC_COUNT = 6;
constexpr int TRAFFIC_COLORS = 3;
constexpr int TRAFFIC_SIZES = 48;
CachedSprite trafficCache
    [TRAFFIC_COLORS]
    [TRAFFIC_SIZES];
constexpr float PLAYER_Z = 40.0f;
constexpr float OVERTAKE_Z = 220.0f;
// TRAFFIC
TrafficCar traffic[TRAFFIC_COUNT];
// ROAD PROJECTION
// FRAMEBUFFER
static inline uint8_t* framebuffer(){
    return (uint8_t*)vga.Buf();
}
// FAST HORIZONTAL SPAN
static inline void drawSpan( uint8_t* line, int x1, int x2, uint8_t color){
    if (x1 < 0)
        x1 = 0;
    if (x2 > W)
        x2 = W;
    if (x2 > x1){
        #if RACER_LIBRARY_SPANS
        vga.hLineLen(x1, (line-framebuffer())/W, x2-x1, color);
#else
        memset(line+x1,color,x2-x1);
#endif
    }
}
// FAST RECTANGLE
static inline void fillRectFast(int x,int y,int w,int h,uint8_t c){
    if(w>0 && h>0) vga.fillRect(x,y,x+w-1,y+h-1,c);
}
// FAST PIXEL
// LINE - FOR TUNNEL
// PROJECT ROAD
// Precompute perspective for all 71 possible horizon heights.
static uint16_t roadScale[71][MAX_DEPTH+1];
static uint8_t roadHalf[71][MAX_DEPTH+1];
static uint16_t depthZ[MAX_DEPTH+1];
static uint8_t depthEdge[MAX_DEPTH+1];
void prepareRoadTables(){
    for(int d=1;d<=MAX_DEPTH;d++){
        depthZ[d]=VIEW_DISTANCE/d; depthEdge[d]=1+d/20;
    }
    for(int h=85;h<=155;h++) for(int d=1;d<=h;d++){
        int t=h-d;
        roadScale[h-85][d]=(uint32_t(t*t)*32768U)/(h*h);
        roadHalf[h-85][d]=d*ROAD_WIDTH/h;
    }
}
RoadPoint projectRoad(int depth,int roadCurve,int roadHorizon){
    int h=H-roadHorizon, row=h-85;
    return {roadHorizon+depth-1, W/2+roadCurve*int(roadScale[row][depth])/32768,
            roadHalf[row][depth],depth};
}
// LOCATION UPDATE
void updateLocation(){
    uint32_t now = millis();
    if (now - locationStart >= LOCATION_TIME){
        locationStart += LOCATION_TIME;
        location = (Location)( ((uint8_t)location + 1) % 9 );
        Serial.println();
        Serial.println("======================");
        Serial.printf( "LOCATION: %s\n", locationNames[(uint8_t)location] );
        Serial.println("======================");
    }
}
// COAST BACKGROUND
void drawCoastBG(){
    // Four indexed sky bands: no per-pixel RGB conversion.
    for(int y=0;y<horizon;y++){
        uint8_t c=P_SKY0+min(3,y*4/horizon);
        memset(framebuffer()+y*W,c,W);
    }
    // Sun descends during sunset, rises during dawn. Clip at horizon.
    int sx=230-curve/3;
    int sy;
    if(dayPhase<512) sy=horizon-55+dayPhase*75/512;
    else sy=horizon+20-(int(dayPhase)-768)*75/256;
    if(dayPhase<512 || dayPhase>=768){
        for(int dy=-22;dy<=22;dy++){
            int y=sy+dy;
            if(y<0 || y>=horizon) continue;
            int hw=(int)sqrtf(484-dy*dy);
            drawSpan(framebuffer()+y*W,sx-hw,sx+hw+1,TUNNEL_LIGHT);
        }
    }
    if(nightAmount>90){
        int mx=85+int(dayPhase)*110/1024-curve/4;
        int my=20+abs(int(dayPhase)-640)/12;
        if(my+11<horizon) vga.fillCircle(mx,my,10,WHITE);
        for(int i=0;i<32;i++){
            int x=(i*73+17)%W, y=(i*31+7)%max(1,horizon-15);
            if(i*8<nightAmount) vga.putPixel(x,y,P_STAR);
        }
    }
}
// MOUNTAIN BACKGROUND
void drawMountainBG(){
    drawCoastBG();
    for(int x=-112-curve/3;x<W;x+=112){
        vga.fillRect(x,horizon-18,x+111,horizon-1,MOUNTAIN_FAR);
        vga.fillTriangle(x,horizon-18,x+56,horizon-74,x+112,horizon-18,MOUNTAIN_FAR);
    }
    for(int x=-156-curve/2;x<W;x+=156){
        int peak=x+78, top=horizon-74, base=horizon-1;
        vga.fillTriangle(x,base,peak,top,peak,base,MOUNTAIN_LIGHT);
        vga.fillTriangle(peak,top,peak,base,x+156,base,MOUNTAIN_NEAR);
        vga.fillTriangle(peak-12,top+12,peak,top,peak+12,top+12,SNOW);
    }
}
// CITY BACKGROUND
void buildCityBG(){
    uint8_t* buf = framebuffer();
    // NIGHT SKY
    for (int y = 0; y < horizon; y++){
        uint8_t c;
        if (y < horizon / 3){
            c = COL332(0,0,64);
        } else if (y < horizon * 2 / 3){
            c = COL332(32,0,64);
        } else {
            c = COL332(64,32,128);
        }
        memset( buf + y * W, c, W );
    }
    // MOON
    int moonX = 250 - curve / 4;
    int moonY = 25;
    vga.fillCircle(moonX,moonY,11,WHITE);
    // STARS
    for (int i = 0; i < 28; i++){
        int x =
            (i * 73 + 17) % W;
        int y =
            (i * 31 + 7) % 45;
        vga.putPixel( x, y, P_STAR );
    }
    // DISTANT CITY
    int shift = curve / 4;
    for (int i = -1; i < 12; i++){
        int x = i * 30 - shift;
        int bh =
            22 + ((i + 12) * 37 % 50);
        int top = horizon - bh;
        fillRectFast( x, top, 27, bh, COL332(32,32,96) );
        for (int yy = top + 5; yy < horizon - 4; yy += 9){
            for (int xx = x + 4; xx < x + 24; xx += 7){
                if (((xx + yy) & 3) != 0){
                    fillRectFast( xx, yy, 2, 3, COL332(96,96,128) );
                }
            }
        }
    }
    // NEAR CITY
    shift = curve / 2;
    for (int i = -2; i < 9; i++){
        int x = i * 48 - shift;
        int bh =
            30 + ((i + 16) * 53 % 65);
        int top = horizon - bh;
        uint8_t building =
            (i & 1)
            ? COL332(32,32,64)
            : COL332(64,32,64);
        fillRectFast( x, top, 40, bh, building );
        // Roof
        fillRectFast( x, top, 40, 2, P_ROOF );
        // Windows
        for (int yy = top + 7; yy < horizon - 4; yy += 10){
            for (int xx = x + 5; xx < x + 35; xx += 9){
                int hash =
                    xx * 13 + yy * 7;
                uint8_t c;
                if ((hash & 7) < 3){
                    c = ((hash >> 3) & 1) ? P_WINDOW0 : P_WINDOW1;
                } else {
                    c = COL332(64,64,128);
                }
                fillRectFast( xx, yy, 4, 5, c );
            }
        }
    }
}
// TUNNEL BACKGROUND
void prepareCityPanorama(){
    int oldH=horizon, oldC=curve;
    horizon=155; curve=0;
    buildCityBG();
    cityReady=cityPanorama.create(W,155,1,true);
    if(cityReady) cityPanorama.getImage(0,0,0);
    horizon=oldH; curve=oldC;
}
void drawCityBG(){
    if(!cityReady){buildCityBG();return;}
    int x=-(curve/3);
    cityPanorama.putImage(x,horizon-155,0);
    cityPanorama.putImage(x+(x>0?-W:W),horizon-155,0);
}
void drawTunnelBG(){
    uint8_t* buf = framebuffer();
    int vx = W / 2 + curve;
    int vy = horizon;
    // CEILING AND WALLS
    for (int y = 0; y < horizon; y++){
        uint8_t* line = buf + y * W;
        int depth = horizon - y;
        int half =
            8 + depth * 170 / horizon;
        int left = vx - half;
        int right = vx + half;
        bool stripe =
            ((4096 / (depth + 1) + gameFrame * 3U) >> 5) & 1;
        uint8_t wall =
            stripe
            ? TUNNEL_WALL1
            : TUNNEL_WALL2;
        drawSpan( line, 0, left, wall );
        drawSpan( line, left, right, TUNNEL_CEILING );
        drawSpan( line, right, W, wall );
    }
    // DISTANT EXIT
    fillRectFast( vx - 8, vy - 12, 16, 12, COL332(96,96,192) );
    // MOVING CEILING LIGHTS
    int phase =
        (gameFrame * 3U) % 128;
    for (int z = 1024 - phase; z >= 32; z -= 128){
        int d = 4096 / z;
        int y = vy - d * 2;
        if (y < -8 || y >= vy)
            continue;
        int lw = 3 + d * 2;
        int lh = 1 + d / 14;
        int cx =
            vx -
            curve * d / 100;
        fillRectFast( cx - lw / 2, y, lw, lh + 2, COL332(96,96,64) );
        fillRectFast( cx - lw / 2, y, lw, lh, P_LAMP );
    }
}
// BACKGROUND SELECTOR
void drawAurora(){
    if(nightAmount<100) return;
    for(int x=0;x<W;x++){
        int y=18+int(9*sinf(x*.035f+gameFrame*.012f))+int(5*sinf(x*.071f-gameFrame*.008f));
        for(int k=0;k<13;k++) if(y+k>=0 && y+k<horizon) framebuffer()[(y+k)*W+x]=P_SEA0+min(3,k/4);
    }
}
void drawDesertBG(){
    drawCoastBG();
    for(int x=0;x<W;x++){
        int top=horizon-18- int(12*sinf(x*.025f+curve*.01f));
        for(int y=max(0,top);y<horizon;y++) framebuffer()[y*W+x]=y<top+5?SAND1:SAND2;
    }
}
void drawNewBG(){
    if(location==Location::NEON){drawTunnelBG(); return;}
    drawCoastBG();
    if(location==Location::FOREST){
        for(int i=-1;i<15;i++){
            int x=i*26-curve/4, h=25+(i+17)*19%48;
            vga.fillTriangle(x-17,horizon-1,x,horizon-h,x+17,horizon-1,COL332(128,64,0));
        }
    }else{
        int x=160-curve/3;
        vga.fillTriangle(x-115,horizon-1,x-18,horizon-75,x+115,horizon-1,COL332(64,32,32));
        vga.fillTriangle(x-18,horizon-75,x+18,horizon-75,x+115,horizon-1,COL332(96,32,0));
        for(int y=max(0,horizon-74);y<horizon;y++){
            int d=y-(horizon-74), cx=x+d/2+int(5*sinf(d*.15f));
            drawSpan(framebuffer()+y*W,cx-2-d/12,cx+3+d/12,P_SEA1);
        }
        for(int i=0;i<8;i++){
            int y=horizon-78-((gameFrame/3+i*11)%65);
            if(y>0) vga.fillCircle(x+int(12*sinf(i+gameFrame*.02f)),y,4+i/2,COL332(96,64,64));
        }
    }
}
static void drawEruption(){
    if(location!=Location::VOLCANO || !eventActive) return;
    int cx=160-curve/3, cy=horizon-75;
    int n=12+eventPower/5;
    for(int i=0;i<n;i++){
        int age=(gameFrame*(1+i%2)+i*19)%64;
        int vx=(i*37%19)-9;
        int x=cx+vx*age/5;
        int y=cy-age*(9+i%5)/8+age*age/48;
        if(y>cy+12) continue;
        int r=1+(i%3==0);
        fillRectFast(x,y,r,r,P_SEA1);
        if(i%4==0) fillRectFast(x,y-2,1,2,P_SEA2);
    }
    for(int i=0;i<5;i++){
        int a=(gameFrame/2+i*13)%65;
        int y=cy-a;
        if(y>0) vga.fillCircle(cx+(i-2)*a/8,y,3+a/9,COL332(96,64,64));
    }
}
void drawWorldBG(){
    switch (location){
        case Location::COAST:
            drawCoastBG();
            break;
        case Location::MOUNTAINS:
            drawMountainBG();
            drawAurora();
            break;
        case Location::TUNNEL:
            drawTunnelBG();
            break;
        case Location::CITY:
            drawCityBG();
            break;
        case Location::DESERT: drawDesertBG(); break;
        case Location::BRIDGE: drawCoastBG(); break;
        case Location::FOREST: case Location::VOLCANO: case Location::NEON: drawNewBG(); break;
    }
}
// COAST SCANLINE
void drawCoastLine( uint8_t* line, const RoadPoint& p, int roadHeight, int z, bool stripe, int edge){
    int depth = p.depth;
    int left =
        p.center - p.halfWidth;
    int right =
        p.center + p.halfWidth;
    int beachWidth =
        4 + depth * 55 / roadHeight;
    int beachRight = left - edge;
    int beachLeft =
        beachRight - beachWidth;
    int wavePhase =
        (z + (gameFrame >> 1)) & 31;
    int waveShift =
        wavePhase < 16
        ? wavePhase
        : 31 - wavePhase;
    waveShift =
        waveShift * depth /
        (roadHeight * 3);
    int seaEdge =
        beachLeft + waveShift;
    uint8_t seaColor =
        depth < 12
        ? P_SEA0
        : (stripe ? P_SEA1 : P_SEA2);
    int wave =
        (z + gameFrame * 2U) & 63;
    if (wave < 1 + depth / 40){
        seaColor = P_WAVE;
    }
    // Sea
    drawSpan( line, 0, seaEdge, seaColor );
    // Beach
    drawSpan( line, seaEdge, beachRight, stripe ? SAND1 : SAND2 );
    // Wet sand
    int wetEnd =
        seaEdge + 1 + depth / 22;
    if (wetEnd > beachRight)
        wetEnd = beachRight;
    drawSpan( line, seaEdge, wetEnd, WET_SAND );
    // Foam
    if (((z + gameFrame) >> 3) & 1){
        int foamEnd = seaEdge + 1;
        if (foamEnd > beachRight)
            foamEnd = beachRight;
        drawSpan( line, seaEdge - 1 - depth / 35, foamEnd, P_FOAM );
    }
    // Right grass
    drawSpan( line, right + edge, W, stripe ? GRASS1 : GRASS2 );
}
// MOUNTAIN SCANLINE
void drawMountainLine( uint8_t* line, const RoadPoint& p, int z, bool stripe, int edge){
    int left =
        p.center - p.halfWidth;
    int right =
        p.center + p.halfWidth;
    uint8_t rock = snowAmount>100 ? (stripe?SNOW:COL332(160,192,224)) : (stripe ? ROCK1 : ROCK2);
    // Rocky ground
    drawSpan( line, 0, left - edge, rock );
    drawSpan( line, right + edge, W, rock );
    // Dark rock bands
    int band =
        ((z + gameFrame * 3U) >> 3) & 7;
    if (band == 0 && snowAmount<100){
        drawSpan( line, 0, left - edge, MOUNTAIN_NEAR );
        drawSpan( line, right + edge, W, MOUNTAIN_NEAR );
    }
    // Gravel near the road
    int gravel =
        2 + p.depth / 10;
    drawSpan( line, left - edge - gravel, left - edge, MOUNTAIN_LIGHT );
    drawSpan( line, right + edge, right + edge + gravel, MOUNTAIN_LIGHT );
}
// TUNNEL SCANLINE
void drawTunnelLine( uint8_t* line, const RoadPoint& p, int z, bool stripe, int edge){
    int left =
        p.center - p.halfWidth;
    int right =
        p.center + p.halfWidth;
    uint8_t wall =
        stripe
        ? TUNNEL_WALL1
        : TUNNEL_WALL2;
    drawSpan( line, 0, left - edge, wall );
    drawSpan( line, right + edge, W, wall );
    // Yellow safety strip
    int strip =
        2 + p.depth / 18;
    drawSpan( line, left - edge - strip, left - edge, TUNNEL_LIGHT );
    drawSpan( line, right + edge, right + edge + strip, TUNNEL_LIGHT );
}
// CITY SCANLINE
void drawCityLine( uint8_t* line, const RoadPoint& p, int z, bool stripe, int edge){
    int left =
        p.center - p.halfWidth;
    int right =
        p.center + p.halfWidth;
    uint8_t ground =
        stripe
        ? CITY_GROUND1
        : CITY_GROUND2;
    drawSpan( line, 0, left - edge, ground );
    drawSpan( line, right + edge, W, ground );
    // Neon sidewalks
    int sidewalk =
        2 + p.depth / 8;
    drawSpan( line, left - edge - sidewalk, left - edge, COL332(96,64,128) );
    drawSpan( line, right + edge, right + edge + sidewalk, COL332(96,64,128) );
    // Neon borders
    drawSpan( line, left - edge - sidewalk, left - edge - sidewalk + 2, P_NEON_PINK );
    drawSpan( line, right + edge + sidewalk - 2, right + edge + sidewalk, P_NEON_BLUE );
}
// COMMON ROAD ENGINE
// RGB332 lighting: preserve existing colors, never paint an opaque cone.
static uint8_t lightLUT[3][256];
void prepareLighting(){
    for(int c=0;c<256;c++){
        int r=(c>>5)&7, g=(c>>2)&7, b=c&3;
        for(int level=0;level<3;level++){
            int rr=min(7,r+level), gg=min(7,g+level);
            int bb=min(3,b+(level==2 ? 1 : 0));
            lightLUT[level][c]=safeBaseIndex((rr<<5)|(gg<<2)|bb);
        }
    }
}
int playerScreenX(){
    int carY=188+((gameFrame>>1)&1);
    int d=constrain(carY+CAR_H-horizon,1,H-horizon);
    RoadPoint p=projectRoad(d,curve,horizon);
    return constrain(p.center-CAR_W/2+(int)(p.halfWidth*playerLane),0,W-CAR_W)+CAR_W/2;
}
// Light only the asphalt, before roadside objects and cars are drawn.
void lightRoadSpan(uint8_t* row,int a,int b,int left,int right,int level){
    a=max(0,max(a,left)); b=min(W,min(b,right));
    const uint8_t* lut=lightLUT[level];
    for(int x=a;x<b;x++) row[x]=lut[row[x]];
}
void drawHeadlightRow(uint8_t* row,const RoadPoint& p,int left,int right,int carX){
    const int nearY=205, farY=horizon+(nearY-horizon)/4;
    if(p.y<farY || p.y>nearY) return;
    int t=(p.y-farY)*256/(nearY-farY);
    // Follow the bend and converge towards the distant road center.
    int cx=p.center+(carX-W/2)*t/256;
    int half=4+24*t/256;
    int separation=2+12*t/256;
    int width=2+half/2;
    for(int side=-1;side<=1;side+=2){
        int c=cx+side*separation;
        lightRoadSpan(row,c-width,c+width,left,right,1);
        lightRoadSpan(row,c-width/2,c+width/2,left,right,t>96?2:1);
    }
}
void drawRoad(){
    uint8_t* buf = framebuffer();
    int roadHeight = H - horizon;
    const bool night=location==Location::NEON || location==Location::VOLCANO || location==Location::TUNNEL || location==Location::CITY || nightAmount>=110;
    const int beamX=night?playerScreenX():0;
    for (int depth = 1; depth <= roadHeight; depth++){
        RoadPoint p =
            projectRoad( depth, curve, horizon );
        int z = depthZ[depth];
        bool stripe =
            ((z + gameFrame * 3U) >> 4) & 1;
        int edge = depthEdge[depth];
        int left =
            p.center - p.halfWidth;
        int right =
            p.center + p.halfWidth;
        uint8_t* line =
            buf + p.y * W;
        // LOCATION ENVIRONMENT
        switch (location){
            case Location::COAST:
                drawCoastLine( line, p, roadHeight, z, stripe, edge );
                break;
            case Location::MOUNTAINS:
                drawMountainLine( line, p, z, stripe, edge );
                break;
            case Location::TUNNEL:
                drawTunnelLine( line, p, z, stripe, edge );
                break;
            case Location::CITY:
                drawCityLine( line, p, z, stripe, edge );
                break;
            case Location::DESERT:
                drawSpan(line,0,W,stripe?SAND1:SAND2); break;
            case Location::BRIDGE:
                drawSpan(line,0,W,stripe?P_SEA1:P_SEA2);
                drawSpan(line,left-edge-5,left-edge,ROCK2);
                drawSpan(line,right+edge,right+edge+5,ROCK2); break;
            case Location::FOREST: drawSpan(line,0,W,stripe?COL332(128,64,0):COL332(96,64,0)); break;
            case Location::VOLCANO:
                drawSpan(line,0,W,stripe?COL332(64,32,32):COL332(96,32,32));
                drawSpan(line,left-edge-depth/3,left-edge-2,P_SEA1);
                drawSpan(line,right+edge+2,right+edge+depth/3,P_SEA2); break;
            case Location::NEON: drawTunnelLine(line,p,z,stripe,edge); break;
        }
        // ASPHALT
        uint8_t road =
            stripe ? ROAD1 : ROAD2;
        uint8_t curb =
            stripe ? RED : WHITE;
        if (location == Location::TUNNEL){
            road =
                stripe
                ? COL332(64,64,64)
                : COL332(96,96,96);
            // Pools of light synchronized with the moving ceiling lamps.
            int phase=(z+gameFrame*3U)&127;
            int dist=min(phase,128-phase);
            road=lightLUT[dist<12?2:(dist<30?1:0)][road];
            curb = TUNNEL_LIGHT;
        }
        if(location==Location::NEON){road=stripe?COL332(32,32,64):COL332(64,32,96); curb=P_SEA0+((gameFrame/12+depth/8)%4);}
        if (location == Location::CITY){
            road =
                stripe
                ? COL332(32,32,64)
                : COL332(64,64,96);
            curb =
                stripe
                ? NEON_BLUE
                : WHITE;
        }
        if(wetAmount>40 && location!=Location::TUNNEL){
            road=stripe?COL332(32,32,64):COL332(64,64,96);
        }
        drawSpan( line, left, right, road );
        if(wetAmount>80 && location==Location::CITY){
            int w=max(1,depth/18); int offset=p.halfWidth*3/4;
            if(((z+gameFrame*3U)&63)<38){
                drawSpan(line,p.center-offset-w,p.center-offset+w,P_NEON_PINK);
                drawSpan(line,p.center+offset-w,p.center+offset+w,P_NEON_BLUE);
            }
        }
        if(wetAmount>80 && ((z+gameFrame*3U)&127)<24){
            int pw=max(1,p.halfWidth/5), px=p.center+p.halfWidth/3;
            drawSpan(line,max(left,px-pw),min(right,px+pw),COL332(64,96,128));
            if(((z+gameFrame*3U)&7)<2) drawSpan(line,max(left,px-pw/2),min(right,px+pw/2),location==Location::CITY?P_NEON_BLUE:P_SEA1);
        }
        // Left curb
        drawSpan( line, left - edge, left, curb );
        // Right curb
        drawSpan( line, right, right + edge, curb );
        // Center road marking
        if (stripe){
            int mark = 1 + depth / 40;
            drawSpan( line, p.center - mark, p.center + mark, WHITE );
        }
        if(night) drawHeadlightRow(line,p,left,right,beamX);
    }
}
// CREATE CACHED SPRITE
bool createCached( CachedSprite& dst, Sprite& source, int w, int h, bool psram){
    dst.sprite =
        new (std::nothrow) Sprite(vga);
    if (!dst.sprite)
        return false;
    bool ok =
        dst.sprite->createScaled( source, w, h, 0, psram );
    // Try PSRAM if internal RAM is full
    if (!ok && !psram){
        ok =
            dst.sprite->createScaled( source, w, h, 0, true );
    }
    if (!ok){
        delete dst.sprite;
        dst.sprite = nullptr;
        return false;
    }
    dst.w = w;
    dst.h = h;
    return true;
}
// DRAW CACHED SPRITE
void drawCached( const CachedSprite& s, int x, int y){
    if (!s.sprite)
        return;
    if (x >= W || y >= H)
        return;
    if (x + s.w <= 0 || y + s.h <= 0)
        return;
    s.sprite->putSprite( x, y, 0, 0 );
}
// PALM CACHE
bool preparePalmCache(){
    Serial.println("Preparing palm cache...");
    uint32_t start = millis();
    size_t bytes = 0;
    for (int depth = MIN_TREE_DEPTH; depth <= MAX_DEPTH; depth++){
        int h = depth * 3 / 2;
        int w = h * PALM_W / PALM_H;
        if (w < 2) w = 2;
        if (h < 2) h = 2;
        if (!createCached( palmCache[depth], palmSprite, w, h, depth > 20 )){
            Serial.println("Palm cache error");
            return false;
        }
        bytes += (size_t)w * h;
        if ((depth % 20) == 0)
            delay(1);
    }
    Serial.printf( "Palm cache: %lu bytes\n", (unsigned long)bytes );
    Serial.printf( "Palm cache time: %lu ms\n", (unsigned long)(millis() - start) );
    return true;
}
// COAST PALMS
void drawTrees(){
    int roadHeight = H - horizon;
    int phase =
        (gameFrame * 3U) % TREE_SPACING;
    for (int z = VIEW_DISTANCE - phase; z >= 24; z -= TREE_SPACING){
        int depth = VIEW_DISTANCE / z;
        if (depth < MIN_TREE_DEPTH || depth > roadHeight || depth > MAX_DEPTH)
            continue;
        RoadPoint p =
            projectRoad( depth, curve, horizon );
        CachedSprite& s =
            palmCache[depth];
        int edge = depthEdge[depth];
        int beachWidth =
            4 + depth * 55 / roadHeight;
        int leftX =
            p.center -
            p.halfWidth -
            edge -
            beachWidth / 2;
        int rightX =
            p.center +
            p.halfWidth +
            3 + depth / 4;
        int y = p.y - s.h + 1;
        int sway=windPower*s.h/55;
        drawCached( s, leftX - s.w / 2+sway, y );
        drawCached( s, rightX - s.w / 2+sway, y );
    }
}
// MOUNTAIN ROCK
void drawRock( int x, int bottom, int size){
    if (size < 2)
        return;
    int h = size;
    int w = size * 3 / 2;
    int top = bottom - h;
    uint8_t* buf = framebuffer();
    for (int yy = 0; yy < h; yy++){
        int y = top + yy;
        if ((unsigned)y >= H)
            continue;
        int half =
            w / 5 +
            yy * w / (h * 3);
        uint8_t* line =
            buf + y * W;
        drawSpan( line, x - half, x, MOUNTAIN_LIGHT );
        drawSpan( line, x, x + half, MOUNTAIN_NEAR );
    }
}
// MOUNTAIN OBJECTS
void drawMountainObjects(){
    int roadHeight = H - horizon;
    int phase =
        (gameFrame * 3U) % 192;
    for (int z = 4096 - phase; z >= 32; z -= 192){
        int depth = VIEW_DISTANCE / z;
        if (depth < 4 || depth > roadHeight)
            continue;
        RoadPoint p =
            projectRoad( depth, curve, horizon );
        int size = 3 + depth / 2;
        int margin = 4 + depth / 3;
        int left =
            p.center -
            p.halfWidth -
            margin;
        int right =
            p.center +
            p.halfWidth +
            margin;
        drawRock( left, p.y, size );
        drawRock( right, p.y, size * 3 / 4 );
    }
}
// TUNNEL ARCHES
void drawTunnelObjects(){
    int roadHeight = H - horizon;
    int phase =
        (gameFrame * 3U) % 160;
    for (int z = 2048 - phase; z >= 32; z -= 160){
        int depth = VIEW_DISTANCE / z;
        if (depth < 3 || depth > roadHeight)
            continue;
        RoadPoint p =
            projectRoad( depth, curve, horizon );
        int half =
            p.halfWidth + 3 + depth / 8;
        int left = p.center - half;
        int right = p.center + half;
        int archHeight = depth * 2;
        int top = p.y - archHeight;
        // Avoid large invisible line loops
        if (top < -H)
            continue;
        int thick = 1 + depth / 50;
        uint8_t archColor =
            COL332(128,128,128);
        // Left support
        fillRectFast( left, top, thick, archHeight, archColor );
        // Right support
        fillRectFast( right, top, thick, archHeight, archColor );
        // Ceiling crossbeam
        fillRectFast( left, top, right - left + thick, thick, archColor );
        // Lamps on the crossbeam
        int lampW = 3 + depth / 2;
        fillRectFast( p.center - lampW / 2, top + thick, lampW, 1 + depth / 30, TUNNEL_LIGHT );
    }
}
// CITY STREET LAMP
void drawStreetLamp( int x, int bottom, int depth, bool leftSide){
    int h = 4 + depth * 3 / 2;
    int top = bottom - h;
    int thick = 1 + depth / 55;
    uint8_t pole =
        COL332(96,96,128);
    fillRectFast( x, top, thick, h, pole );
    int arm = 2 + depth / 5;
    int lampX =
        leftSide
        ? x
        : x - arm;
    fillRectFast( lampX, top, arm, thick, pole );
    int lampW = 2 + depth / 12;
    int lightX =
        leftSide
        ? x + arm - lampW
        : x - arm;
    // Neon glow
    fillRectFast( lightX - 1, top + thick, lampW + 2, thick + 2, P_NEON_BLUE );
    // Bright center
    fillRectFast( lightX, top + thick, lampW, thick, WHITE );
}
// CITY NEON SIGNS
void drawNeonSign( int x, int bottom, int depth, uint8_t color){
    int w = 3 + depth / 2;
    int h = 2 + depth / 5;
    int top = bottom - h * 3;
    int poleW = 1 + depth / 50;
    fillRectFast( x, top + h, poleW, bottom - top - h, COL332(96,96,96) );
    fillRectFast( x - w / 2, top, w, h, color );
    if (w > 4 && h > 3){
        fillRectFast( x - w / 2 + 1, top + 1, w - 2, h - 2, COL332(32,0,64) );
        fillRectFast( x - w / 3, top + h / 2, w * 2 / 3, 1, WHITE );
    }
}
// CITY OBJECTS
void drawCityObjects(){
    int roadHeight = H - horizon;
    int phase =
        (gameFrame * 3U) % 128;
    for (int z = 4096 - phase; z >= 32; z -= 128){
        int depth = VIEW_DISTANCE / z;
        if (depth < 4 || depth > roadHeight)
            continue;
        RoadPoint p =
            projectRoad( depth, curve, horizon );
        int margin = 4 + depth / 5;
        int left =
            p.center -
            p.halfWidth -
            margin;
        int right =
            p.center +
            p.halfWidth +
            margin;
        drawStreetLamp( left, p.y, depth, true );
        drawStreetLamp( right, p.y, depth, false );
        if ((z / 128) & 1){
            drawNeonSign( left - depth / 3, p.y, depth, P_NEON_PINK );
            drawNeonSign( right + depth / 3, p.y, depth, P_NEON_BLUE );
        }
    }
}
// LOCATION OBJECTS
void drawExtraObjects(bool bridge){
    int phase=(gameFrame*3U)%256;
    for(int z=4096-phase;z>=28;z-=256){
        int d=4096/z; if(d<4 || d>H-horizon) continue;
        RoadPoint p=projectRoad(d,curve,horizon);
        int h=max(3,d*(bridge?2:1));
        int a=p.center-p.halfWidth-5-d/8,b=p.center+p.halfWidth+5+d/8;
        for(int side=0;side<2;side++){
            int x=side?b:a;
            fillRectFast(x,p.y-h,max(1,d/12),h,bridge?ROCK2:GRASS1);
            if(!bridge){fillRectFast(x-d/7,p.y-h/2,max(2,d/3),max(1,d/10),GRASS1);}
        }
        if(bridge){fillRectFast(a,p.y-h,b-a+1,max(1,d/15),ROCK2);}
    }
}
void drawNewObjects(){
    int phase=(gameFrame*3U)%192;
    for(int z=4096-phase;z>=28;z-=192){
        int d=4096/z; if(d<4 || d>H-horizon) continue;
        RoadPoint p=projectRoad(d,curve,horizon);
        int a=p.center-p.halfWidth-d/4-4,b=p.center+p.halfWidth+d/4+4;
        if(location==Location::NEON){
            int top=p.y-d*2, thick=max(1,d/18); uint8_t c=P_SEA0+((z/192+gameFrame/12)%4);
            fillRectFast(a,top,thick,d*2,c); fillRectFast(b,top,thick,d*2,c);
            fillRectFast(a,top,b-a+thick,thick,c);
        }else for(int side=0;side<2;side++){
            int x=side?b:a, h=max(3,d*2), w=max(2,d/2);
            if(location==Location::FOREST){
                fillRectFast(x,p.y-h/2,max(1,d/9),h/2,ROCK1);
                vga.fillTriangle(x-w,p.y-h/3,x,p.y-h,x+w,p.y-h/3,(z/192+side)%2?COL332(224,96,0):COL332(224,160,0));
            }else vga.fillTriangle(x-w,p.y,x,p.y-h/2,x+w,p.y,COL332(96,32,32));
        }
    }
}
void drawWorldObjects(){
    switch (location){
        case Location::COAST:
            drawTrees();
            break;
        case Location::MOUNTAINS:
            drawMountainObjects();
            break;
        case Location::TUNNEL:
            drawTunnelObjects();
            break;
        case Location::CITY:
            drawCityObjects();
            break;
        case Location::DESERT: drawExtraObjects(false); break;
        case Location::BRIDGE: drawExtraObjects(true); break;
        case Location::FOREST: case Location::VOLCANO: case Location::NEON: drawNewObjects(); break;
    }
}
// RECOLOR TRAFFIC CAR
uint8_t recolorCar( uint8_t c, int color){
    if (c == 0)
        return 0;
    int r = (c >> 5) & 7;
    int g = (c >> 2) & 7;
    int b = c & 3;
    if (r < 3 || r <= g + 1 || r <= b * 2)
        return c;
    switch (color){
        case 0:
            // Yellow
            g = r;
            b = 0;
            break;
        case 1:
            // Blue
            b = (r + 1) / 2;
            if (b > 3)
                b = 3;
            g = r / 2;
            r = 0;
            break;
        case 2:
            // White
            g = r;
            b = r * 3 / 7;
            break;
    }
    return safeBaseIndex((r << 5) | (g << 2) | b);
}
// TRAFFIC CACHE
bool prepareTrafficCache(){
    Serial.println("Preparing traffic cache...");
    static uint8_t image[
        CAR_W * CAR_H
    ];
    size_t bytes = 0;
    uint32_t start = millis();
    for (int color = 0; color < TRAFFIC_COLORS; color++){
        for (int i = 0; i < CAR_W * CAR_H; i++){
            image[i] =
                recolorCar( car[1][i], color );
        }
        Sprite source(vga);
        if (!source.create( CAR_W, CAR_H, 1, false ))
            return false;
        if (!source.load(image))
            return false;
        for (int i = 0; i < TRAFFIC_SIZES; i++){
            int h = 3 + i * 2;
            int w =
                h * CAR_W / CAR_H;
            if (w < 2)
                w = 2;
            if (!createCached( trafficCache[color][i], source, w, h, i >= 8 )){
                return false;
            }
            bytes += (size_t)w * h;
            if ((i & 15) == 0)
                delay(1);
        }
        Serial.printf( "Traffic color %d OK\n", color );
    }
    Serial.printf( "Traffic cache: %lu bytes\n", (unsigned long)bytes );
    Serial.printf( "Traffic cache time: %lu ms\n", (unsigned long)(millis() - start) );
    return true;
}
// INIT TRAFFIC
void initTraffic(){
    for (int i = 0; i < TRAFFIC_COUNT; i++){
        TrafficCar& c = traffic[i];
        c.z = 180 + i * 320;
        c.lane =
            (i & 1)
            ? 0.5f
            : -0.5f;
        c.speed =
            1.5f + (i % 3) * 0.65f;
        c.color =
            i % TRAFFIC_COLORS;
        c.passed = false;
    }
    playerLane = 0.0f;
    targetLane = 0.0f;
    overtakes = 0;
}
// UPDATE TRAFFIC
void updateTraffic(){
    for (int i = 0; i < TRAFFIC_COUNT; i++){
        TrafficCar& c = traffic[i];
        c.z -= c.speed;
        if (c.z < PLAYER_Z && !c.passed){
            c.passed = true;
            overtakes++;
        }
        if (c.z < 20.0f){
            c.z =
                1700 +
                random(0,1800);
            c.lane =
                random(0,2)
                ? 0.5f
                : -0.5f;
            c.speed =
                random(120,300) /
                100.0f;
            c.color =
                random( 0, TRAFFIC_COLORS );
            c.passed = false;
        }
    }
    // Find nearest car
    float nearest = 10000.0f;
    int nearestCar = -1;
    for (int i = 0; i < TRAFFIC_COUNT; i++){
        TrafficCar& c = traffic[i];
        if (c.z > 25.0f && c.z < nearest){
            nearest = c.z;
            nearestCar = i;
        }
    }
    // Automatic overtaking
    targetLane = 0.0f;
    if (nearestCar >= 0 && nearest < OVERTAKE_Z){
        targetLane =
            traffic[nearestCar].lane < 0
            ? 0.5f
            : -0.5f;
    }
    playerLane +=
        (targetLane - playerLane)
        * 0.035f;
}
static void drawSpray(int x,int y,int w);
// DRAW TRAFFIC CAR
void drawTrafficCar( const TrafficCar& c){
    if (c.z <= 0)
        return;
    int roadHeight = H - horizon;
    int playerDepth =
        188 + CAR_H - horizon;
    int depth =
        (int)( playerDepth * PLAYER_Z / c.z );
    if (depth < 3 || depth > roadHeight)
        return;
    RoadPoint p =
        projectRoad( depth, curve, horizon );
    int h =
        depth * CAR_H /
        playerDepth;
    if (h < 3)
        h = 3;
    int index =
        constrain( (h - 3) / 2, 0, TRAFFIC_SIZES - 1 );
    CachedSprite& s =
        trafficCache[c.color][index];
    int x =
        p.center +
        (int)( p.halfWidth * c.lane );
    int y =
        p.y - s.h + 1;
    drawCached( s, x - s.w / 2, y );
    drawSpray(x,p.y,s.w);
}
// DRAW ALL TRAFFIC
void drawTraffic(){
    uint8_t order[TRAFFIC_COUNT];
    for (int i = 0; i < TRAFFIC_COUNT; i++){
        order[i] = i;
    }
    // Far to near
    for (int i = 1; i < TRAFFIC_COUNT; i++){
        uint8_t key = order[i];
        int j = i - 1;
        while ( j >= 0 && traffic[order[j]].z < traffic[key].z ){
            order[j + 1] =
                order[j];
            j--;
        }
        order[j + 1] = key;
    }
    for (int i = 0; i < TRAFFIC_COUNT; i++){
        drawTrafficCar( traffic[order[i]] );
    }
}
// DRAW PLAYER
void drawPlayer(){
    uint8_t carIndex = 1;
    float steering =
        targetLane - playerLane;
    if (steering < -0.08f){
        carIndex = 0;
    } else if (steering > 0.08f){
        carIndex = 2;
    } else {
        if (curve < -25)
            carIndex = 0;
        if (curve > 25)
            carIndex = 2;
    }
    int carY =
        188 +
        ((gameFrame >> 1) & 1);
    int depth =
        carY + CAR_H - horizon;
    depth =
        constrain( depth, 1, H - horizon );
    RoadPoint p =
        projectRoad( depth, curve, horizon );
    int carX =
        p.center -
        CAR_W / 2 +
        (int)( p.halfWidth * playerLane );
    carX =
        constrain( carX, 0, W - CAR_W );
    // Compressed, dithered reflection of the actual car sprite.
    if(wetAmount>80){
        const uint8_t* src=car[carIndex];
        for(int dy=0;dy<5;dy++){
            int y=carY+CAR_H+dy; if(y>=H) break;
            int sy=CAR_H-1-dy*5;
            for(int x=0;x<CAR_W;x++){
                uint8_t c=src[sy*CAR_W+x];
                if(c && ((x+dy+gameFrame/3)&1)==0) framebuffer()[y*W+carX+x]=lightLUT[0][c];
            }
        }
    }
    carSprite.putSprite( carX, carY, 0, carIndex );
    // TUNNEL LIGHT REFLECTION
    if (location == Location::TUNNEL){
        if (((gameFrame * 3U) & 63) < 12){
            fillRectFast( carX + 22, carY + 3, 28, 2, TUNNEL_LIGHT );
        }
    }
}
// UPDATE GAME
void updateGame(){
    updateLocation();
    updateWorldEvent();
    int curveLimit = MAX_CURVE;
    int hillLimit = MAX_HILL;
    // Different road behavior per location
    switch (location){
        case Location::COAST:
            curveLimit = 110;
            hillLimit = 35;
            break;
        case Location::MOUNTAINS:
            curveLimit = 125;
            hillLimit = 35;
            break;
        case Location::TUNNEL:
            curveLimit = 28;
            hillLimit = 8;
            break;
        case Location::CITY:
            curveLimit = 65;
            hillLimit = 12;
            break;
        case Location::DESERT: curveLimit=85; hillLimit=22; break;
        case Location::BRIDGE: curveLimit=18; hillLimit=8; break;
        case Location::FOREST: curveLimit=100; hillLimit=25; break;
        case Location::VOLCANO: curveLimit=75; hillLimit=25; break;
        case Location::NEON: curveLimit=25; hillLimit=8; break;
    }
    // Smooth changes when entering locations
    int targetCurve =
        (int)( sinf(gameFrame * 0.008f) * curveLimit );
    int targetHill =
        (int)( sinf(gameFrame * 0.005f) * hillLimit );
    // Integer smoothing with minimum step
    int dc = targetCurve - curve;
    int dh = targetHill - hill;
    if (dc != 0){
        int step = dc / 8;
        if (step == 0)
            step = dc > 0 ? 1 : -1;
        curve += step;
    }
    if (dh != 0){
        int step = dh / 8;
        if (step == 0)
            step = dh > 0 ? 1 : -1;
        hill += step;
    }
    horizon =
        constrain( BASE_HORIZON + hill, BASE_HORIZON - MAX_HILL, BASE_HORIZON + MAX_HILL );
    if(location==Location::VOLCANO && eventPower>100){
        int shake=eventPower/85;
        horizon=constrain(horizon+int(weatherRand()%(2*shake+1))-shake,BASE_HORIZON-MAX_HILL,BASE_HORIZON+MAX_HILL);
    }
    updateTraffic();
}
// LOCATION PROGRESS BAR
void drawLocationProgress(){
    uint32_t elapsed =
        millis() - locationStart;
    if (elapsed > LOCATION_TIME)
        elapsed = LOCATION_TIME;
    int barWidth =
        elapsed * 100UL /
        LOCATION_TIME;
    int x = 210;
    int y = 38;
    fillRectFast( x, y, 100, 4, COL332(64,64,64) );
    fillRectFast( x, y, barWidth, 4, NEON_BLUE );
}
// HUD
void drawHUD(){
    vga.putPrintf( 5, 5, WHITE, "FPS: %.1f", vga.FPS() );
    vga.putPrintf( 200, 5, WHITE, "%d KM/H", speed );
    uint8_t titleColor = WHITE;
    if (location == Location::TUNNEL){
        titleColor = TUNNEL_LIGHT;
    }
    if (location == Location::CITY){
        titleColor = NEON_BLUE;
    }
    vga.putPrintf( 5, 20, titleColor, "%s", locationNames[(uint8_t)location] );
    vga.putPrintf( 200, 20, WHITE, "PASS: %lu", (unsigned long)overtakes );
    if (hill < -10){
        vga.putPrintf( 5, 35, WHITE, "UPHILL" );
    } else if (hill > 10){
        vga.putPrintf( 5, 35, WHITE, "DOWNHILL" );
    }
    if (fabsf(targetLane) > 0.1f){
        vga.putPrintf( 5, 50, WHITE, "OVERTAKING" );
    }
    vga.putPrintf(5,65,WHITE,"%s",dayPhase<256?"SUNSET":dayPhase<512?"DUSK":dayPhase<768?"NIGHT":"DAWN");
    if(snowAmount) vga.putPrintf(5,80,WHITE,"SNOW %u%%",(unsigned)(snowAmount*100/255));
    else if(rainAmount) vga.putPrintf(5,80,WHITE,lightning?"THUNDER":"RAIN %u%%",(unsigned)(rainAmount*100/255));
    if(sandAmount) vga.putPrintf(5,80,WHITE,"SAND %u%%",(unsigned)(sandAmount*100/255));
    if(eventActive) vga.putPrintf(5,95,WHITE,"%s",eventNames[(int)location]);
    drawLocationProgress();
    // Seconds until next location
    uint32_t elapsed =
        millis() - locationStart;
    if (elapsed > LOCATION_TIME)
        elapsed = LOCATION_TIME;
    int remaining =
        (LOCATION_TIME - elapsed + 999)
        / 1000;
    vga.putPrintf( 210, 46, WHITE, "NEXT: %02d", remaining );
}
// RENDER GAME
// Portal approaches during final 4 seconds of mountain stage.
// The city is revealed through an expanding tunnel mouth on exit.
void drawTransition(){
    uint32_t elapsed=millis()-locationStart;
    if(location==Location::MOUNTAINS && elapsed>=16000){
        int t=constrain(int(elapsed-16000),0,4000);
        int d=3+int(152LL*t*t/16000000LL);
        d=constrain(d,1,H-horizon);
        RoadPoint p=projectRoad(d,curve,horizon);
        int half=p.halfWidth+4+d/6;
        int top=horizon-8-d*2;
        int bottom=p.y;
        int x=p.center;
        // Arched portal: scanline silhouette, road floor and ceiling lights.
        int arch=max(4,half/2), spring=top+arch;
        int border=max(3,d/8);
        for(int y=max(0,top-border);y<min(H,bottom);y++){
            int dy=max(0,spring-y);
            int outer=half+border;
            int inner=half;
            if(dy>0){
                int a=arch+border;
                outer=dy>=a?0:int((half+border)*sqrtf(1.0f-float(dy*dy)/float(a*a)));
                inner=dy>=arch?0:int(half*sqrtf(1.0f-float(dy*dy)/float(arch*arch)));
            }
            uint8_t* row=framebuffer()+y*W;
            drawSpan(row,x-outer,x+outer,MOUNTAIN_NEAR);
            drawSpan(row,x-inner,x+inner,BLACK);
            if(y>=horizon && inner>0){
                int floorHalf=min(inner,(y-horizon)*half/max(1,bottom-horizon));
                drawSpan(row,x-floorHalf,x+floorHalf,ROAD1);
                if(floorHalf>3) drawSpan(row,x-1,x+1,WHITE);
            }
        }
        for(int i=1;i<=4;i++){
            int yy=horizon-(horizon-top)*i/5;
            int ww=max(1,half*i/8);
            if(yy>=0 && yy<bottom) drawSpan(framebuffer()+yy*W,x-ww,x+ww,P_LAMP);
        }
    } else if(location==Location::CITY && elapsed<3000){
        int t=int(elapsed);
        int half=12+t*200/3000;
        int up=10+t*170/3000;
        int cx=W/2+curve*(3000-t)/3000;
        int top=horizon-up;
        // Only the city visible through the growing exit; road remains continuous.
        fillRectFast(0,0,W,top,TUNNEL_CEILING);
        fillRectFast(0,top,cx-half,horizon-top,TUNNEL_WALL2);
        fillRectFast(cx+half,top,W-cx-half,horizon-top,TUNNEL_WALL2);
    }
}
// Live palette: RGB332 bytes remain indices, RGB565 output changes.
// 0 = static RGB332-equivalent palette, 1 = animated palette.
#define RACER_ANIMATE_PALETTE 1
static uint8_t paletteShade = 255;
static uint8_t lastPaletteShade = 0;
static bool paletteInitialized = false;

void updateLivePalette(){
    dayPhase=(uint32_t)(millis()-dayStart)%DAY_CYCLE_MS*1024UL/DAY_CYCLE_MS;
    int phase=dayPhase;
    int shade=255;
#if RACER_ANIMATE_PALETTE
    if(phase<256) nightAmount=phase*100/256;
    else if(phase<512) nightAmount=100+(phase-256)*155/256;
    else if(phase<768) nightAmount=255;
    else nightAmount=255-(phase-768)*255/256;
    shade=255-nightAmount*120/255;
#endif
    // Sky colors interpolate from sunset to midnight independently of location.
    const uint8_t warm[4][3]={{105,45,170},{185,55,155},{245,85,105},{255,175,100}};
    const uint8_t dark[4][3]={{5,8,35},{12,18,60},{25,32,85},{55,45,100}};
    static int lastSky=-1;
    int mix=min(255,int(nightAmount)+int(rainAmount)/3)&~3;
    if(lightning) mix=0;
    if(mix!=lastSky){
        lastSky=mix;
        for(int i=0;i<4;i++) vga.setPalCol(P_SKY0+i,
            (warm[i][0]*(255-mix)+dark[i][0]*mix)/255,
            (warm[i][1]*(255-mix)+dark[i][1]*mix)/255,
            (warm[i][2]*(255-mix)+dark[i][2]*mix)/255);
    }
    shade=max(65,shade-int(wetAmount)*35/255);
    if(location==Location::TUNNEL && eventActive) shade=max(35,shade-int(eventPower)*150/255);
    if(location==Location::VOLCANO && eventActive) shade=min(255,shade+int(eventPower)/3);
    if(lightning) shade=min(255,shade+int(lightning));
    // Quantize updates to reduce palette writes; keep transitions gradual.
    shade = (shade + 2) & ~3;
    if (shade > 255) shade = 255;
    paletteShade = (uint8_t)shade;
    if (paletteInitialized && paletteShade == lastPaletteShade) return;
    paletteInitialized = true;
    lastPaletteShade = paletteShade;
    for (int i = 0; i < 256; ++i){
        if (i >= P_SEA0 && i <= P_SKY3) continue;
        // Match the library's original RGB332 -> RGB565 table exactly.
        uint16_t c = TABLE_332to565[i];
        int r = ((c >> 11) & 31) * 255 / 31;
        int g = ((c >> 5) & 63) * 255 / 63;
        int b = (c & 31) * 255 / 31;
        // Shared indices: keep white, bright neon and lamps readable.
        int gain = shade;
        if (i == WHITE || i == NEON_BLUE || i == NEON_PINK ||
            i == TUNNEL_LIGHT || i == WINDOW_YELLOW) gain = 255;
        r = (r * gain + 127) / 255;
        g = (g * gain + 127) / 255;
        b = (b * gain + 127) / 255;
        vga.setPalCol((uint8_t)i, r, g, b);
    }
}

// Animate only palette entries; cached city and sprites stay untouched.
void animateWorldPalette(){
    if ((gameFrame & 3U) != 0) return;
#if !RACER_ANIMATE_PALETTE
    if (gameFrame != 0) return;
#endif
    const float t = millis() * 0.001f;
    const float wave = 0.5f + 0.5f*sinf(t*2.4f);
    const float wave2 = 0.5f + 0.5f*sinf(t*2.4f+2.1f);
    const float pulse = 0.5f + 0.5f*sinf(t*3.0f);
    const int shade = paletteShade;
    vga.setPalCol(P_SEA0, 0, (70+20*wave)*shade/255, (190+25*wave)*shade/255);
    vga.setPalCol(P_SEA1, 0, (120+35*wave2)*shade/255, (220+30*wave2)*shade/255);
    vga.setPalCol(P_SEA2, 0, (150+30*wave)*shade/255, (225+30*wave)*shade/255);
    vga.setPalCol(P_WAVE, (80+80*wave)*shade/255, (180+60*wave)*shade/255, 255*shade/255);
    vga.setPalCol(P_FOAM, (160+70*wave2)*shade/255, (210+40*wave2)*shade/255, shade);
    vga.setPalCol(P_NEON_PINK, 210+45*pulse, 25+65*pulse, 150+100*pulse);
    vga.setPalCol(P_NEON_BLUE, 0, 155+100*wave2, 255);
    vga.setPalCol(P_ROOF, 15, 140+100*pulse, 240);
    vga.setPalCol(P_LAMP, 255, 210+35*wave, 115+65*wave);
    vga.setPalCol(P_WINDOW0, 255, 190+45*wave2, 95+40*wave2);
    vga.setPalCol(P_WINDOW1, 220+35*pulse, 160+75*pulse, 75+65*pulse);
    vga.setPalCol(P_STAR, 100+130*wave, 120+120*wave, 180+75*wave);
    if(location==Location::TUNNEL && eventActive){
        int g=255-eventPower*3/4;
        vga.setPalCol(P_LAMP,g,g*4/5,g/2);
    }
    if(location==Location::CITY && eventActive && eventPower>180){
        vga.setPalCol(P_WINDOW0,45,35,25);
        vga.setPalCol(P_WINDOW1,30,25,20);
    }
    if(location==Location::VOLCANO){
        vga.setPalCol(P_SEA1,255,65+int(150*wave),0);
        vga.setPalCol(P_SEA2,255,30+int(100*wave2),0);
    }
    if(location==Location::NEON){
        for(int i=0;i<4;i++){
            float a=t*(2+eventPower/45.0f)+i*1.57f;
            vga.setPalCol(P_SEA0+i,128+int(127*sinf(a)),128+int(127*sinf(a+2.094f)),128+int(127*sinf(a+4.188f)));
        }
    }
    if(location==Location::MOUNTAINS && nightAmount>=100){
        vga.setPalCol(P_SEA0,20,80+70*wave,100);
        vga.setPalCol(P_SEA1,35,150+90*wave2,120);
        vga.setPalCol(P_SEA2,80,200+50*pulse,160);
        vga.setPalCol(P_WAVE,120+80*wave,65,190+60*pulse);
    }
}

// Screen-space rain, clipped direct writes to the back buffer.
struct WeatherDrop { int16_t x,y; uint8_t speed,len; };
static WeatherDrop drops[RAIN_COUNT];
static bool dropsReady=false;
static void drawRain(){
    if(!rainAmount && !snowAmount && !sandAmount) return;
    uint8_t* buf=framebuffer();
    if(!dropsReady){
        for(int i=0;i<RAIN_COUNT;i++) drops[i]={(int16_t)(weatherRand()%W),(int16_t)(weatherRand()%H),(uint8_t)(5+weatherRand()%8),(uint8_t)(4+weatherRand()%7)};
        dropsReady=true;
    }
    int count=RAIN_COUNT*max(max(rainAmount,snowAmount),sandAmount)/255;
    int wind=curve/35+2+windPower;
    for(int i=0;i<count;i++){
        auto &d=drops[i]; int layer=i%3;
        if(sandAmount){d.x=(d.x+W+5+windPower+layer)%W; d.y=(d.y+H+1+layer)%H; for(int k=0;k<3;k++) if(d.x+k<W) buf[d.y*W+d.x+k]=layer?SAND1:SAND2; continue;}
         bool flake=(i%255)<snowAmount; d.x+=flake?(int((gameFrame/16+i)%3)-1+windPower/2):wind; d.y+=flake?1+layer:d.speed+layer*3;
        if(d.y>=H+12){d.y=-12; d.x=weatherRand()%W;}
        if(d.x<0) d.x+=W; if(d.x>=W) d.x-=W;
        if(flake){
            if((unsigned)d.x<W && (unsigned)d.y<H) buf[d.y*W+d.x]=layer==0?SNOW:WHITE;
            if(layer==2 && d.x+1<W && d.y+1<H && d.y>=0){buf[d.y*W+d.x+1]=WHITE;buf[(d.y+1)*W+d.x]=WHITE;}
            continue;
        }
        for(int k=0;k<d.len+layer*2;k++){
            int x=d.x+wind*k/4,y=d.y+k;
            if((unsigned)x<W && (unsigned)y<H) buf[y*W+x]=(k<2?P_STAR:P_SEA2);
        }
    }
    for(int i=0;i<RAIN_SPLASHES*rainAmount/255;i++){
        int d=1+weatherRand()%(H-horizon); RoadPoint p=projectRoad(d,curve,horizon);
        int x=p.center-p.halfWidth+weatherRand()%max(1,2*p.halfWidth);
        if((unsigned)x<W) buf[p.y*W+x]=P_STAR;
        if(x>0 && x<W && p.y>0) buf[(p.y-1)*W+x-1]=P_SEA2;
    }
}
// Extra effects use the existing back buffer, no per-frame allocation.
static void weatherLine(int x,int y,int x2,int y2,uint8_t c){
    int dx=abs(x2-x),dy=-abs(y2-y),sx=x<x2?1:-1,sy=y<y2?1:-1,e=dx+dy;
    for(;;){if((unsigned)x<W && (unsigned)y<H) framebuffer()[y*W+x]=c;
        if(x==x2 && y==y2) break; int q=2*e; if(q>=dy){e+=dy;x+=sx;} if(q<=dx){e+=dx;y+=sy;}}
}
static void drawBolt(){
    if(!lightning) return;
    int x=70+(gameFrame/12*37)%180,y=0;
    for(int i=0;i<7;i++){int nx=x+((i&1)?-12:17),ny=y+max(3,horizon/9);
        weatherLine(x,y,nx,ny,WHITE); weatherLine(x+1,y,nx+1,ny,P_STAR);
        if(i==2 || i==4) weatherLine(x,y,x-25,y+20,P_STAR); x=nx;y=ny;}
}
static void drawSpray(int x,int y,int w){
    if(!rainAmount && !snowAmount) return;
    int n=max(2,w/3)*max(rainAmount,snowAmount)/255;
    for(int i=0;i<n;i++){int side=(i&1)?1:-1; int px=x+side*(w/3+int(weatherRand()%max(1,w/4))); int py=y+int(weatherRand()%max(2,w/5));
        if((unsigned)px<W && (unsigned)py<H) framebuffer()[py*W+px]=snowAmount>100?SNOW:P_FOAM;}
}
static void drawWeatherFog(){
    if(!fogAmount) return;
    // Ordered dither, restricted to distant scenery around the horizon.
    int radius=8+fogAmount/3;
    for(int y=max(0,horizon-radius);y<min(H,horizon+radius);y++){
        int strength=(radius-abs(y-horizon))*fogAmount/radius;
        uint8_t* row=framebuffer()+y*W;
        for(int x=0;x<W;x+=2) if(((x*13+y*7)&63)<strength/4) row[x]=sandAmount?SAND2:COL332(160,192,224);
    }
}
void renderGame(){
    uint32_t t0 = micros();
    updateWeather();
    updateLivePalette();
    animateWorldPalette();
    uint32_t tp = micros();
    // 1. Background
    drawWorldBG();
    drawEruption();
    drawBolt();
    uint32_t t1 = micros();
    // 2. Road
    drawRoad();
    uint32_t t2 = micros();
    // 3. Location objects
    drawWorldObjects();
    drawTransition();
    uint32_t t3 = micros();
    // 4. Traffic
    drawTraffic();
    uint32_t t4 = micros();
    // 5. Player
    drawWeatherFog();
    drawPlayer();
    drawSpray(playerScreenX(),230,CAR_W);
    uint32_t t5 = micros();
    drawRain();
    if(location==Location::FOREST || location==Location::VOLCANO){
        for(int i=0;i<100+eventPower/2;i++){
            int x=(i*73+gameFrame*(1+i%3))%W;
            int y=(i*47+gameFrame*(1+i%2))%H;
            if(location==Location::VOLCANO) y=H-1-y;
            uint8_t c=location==Location::VOLCANO?P_SEA1:COL332(224,96+(i%3)*32,0);
            framebuffer()[y*W+x]=c;
            if(i%3==0 && x+1<W) framebuffer()[y*W+x+1]=c;
        }
    }
    uint32_t tr = micros();
    // 6. HUD
    drawHUD();
    uint32_t t6 = micros();
    // 7. Swap
    vga.swap();
    uint32_t t7 = micros();
    // PERFORMANCE
    if ((gameFrame % 60) == 0){
        Serial.println( "-------------------------" );
        Serial.printf( "LOCATION: %s\n", locationNames[(uint8_t)location] );
        Serial.printf( "FRAME:   %lu\n", (unsigned long)gameFrame );
        Serial.printf( "HORIZON: %d\n", horizon );
        Serial.printf( "PASS:    %lu\n", (unsigned long)overtakes );
        Serial.printf("DAY: %lu s, night %u\n", (unsigned long)((millis()-dayStart)%DAY_CYCLE_MS/1000), (unsigned)nightAmount);
        Serial.printf( "PAL:     %6.3f ms (shade %u)\n", (tp - t0) / 1000.0f, (unsigned)paletteShade );
        Serial.printf( "BG:      %6.3f ms\n", (t1 - tp) / 1000.0f );
        Serial.printf( "ROAD:    %6.3f ms\n", (t2 - t1) / 1000.0f );
        Serial.printf( "OBJECTS: %6.3f ms\n", (t3 - t2) / 1000.0f );
        Serial.printf( "TRAFFIC: %6.3f ms\n", (t4 - t3) / 1000.0f );
        Serial.printf( "CAR:     %6.3f ms\n", (t5 - t4) / 1000.0f );
        Serial.printf("WEATHER: %6.3f ms (%u particles)\n", (tr-t5)/1000.0f, (unsigned)(RAIN_COUNT*max(max(rainAmount,snowAmount),sandAmount)/255));
        Serial.printf( "HUD:     %6.3f ms\n", (t6 - tr) / 1000.0f );
        Serial.printf( "RENDER:  %6.3f ms\n", (t6 - t0) / 1000.0f );
        Serial.printf( "SWAP:    %6.3f ms\n", (t7 - t6) / 1000.0f );
        Serial.printf( "TOTAL:   %6.3f ms\n", (t7 - t0) / 1000.0f );
    }
}
// INIT SPRITES
bool initSprites(){
    // Player car
    if (!carSprite.create( CAR_W, CAR_H, 3, false )){
        Serial.println("CAR CREATE ERROR");
        return false;
    }
    for (int i = 0; i < 3; i++){
        if (!carSprite.load( car[i], i )){
            Serial.println("CAR LOAD ERROR");
            return false;
        }
    }
    Serial.println("Car OK");
    // Palm
    if (!palmSprite.create( PALM_W, PALM_H, 1, false )){
        Serial.println("PALM CREATE ERROR");
        return false;
    }
    if (!palmSprite.load(palm)){
        Serial.println("PALM LOAD ERROR");
        return false;
    }
    Serial.println("Palm OK");
    // Cached sprites
    if (!preparePalmCache())
        return false;
    if (!prepareTrafficCache())
        return false;
    return true;
}
// VGA initialization on core 0; Arduino setup/loop normally run on core 1.
static SemaphoreHandle_t vgaInitDone = nullptr;
static bool vgaInitOK = false;

static void vgaInitTask(void* arg){
    Serial.printf("[VGA] INIT CORE: %d\n", xPortGetCoreID());
    vgaInitOK = vga.init(Mode320x240, ColorDepth::BPP_8, true, true);
    xSemaphoreGive(vgaInitDone);
    vTaskDelete(nullptr);
}

static bool initVGACore0(){
    vgaInitDone = xSemaphoreCreateBinary();
    if (!vgaInitDone) return false;
    if (xTaskCreatePinnedToCore(vgaInitTask, "VGA_INIT", 8192,
                                nullptr, 5, nullptr, 0) != pdPASS){
        vSemaphoreDelete(vgaInitDone);
        vgaInitDone = nullptr;
        return false;
    }
    xSemaphoreTake(vgaInitDone, portMAX_DELAY);
    vSemaphoreDelete(vgaInitDone);
    vgaInitDone = nullptr;
    return vgaInitOK;
}

// SETUP
void setup(){
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("======================");
    Serial.println("      VGA RACER");
    Serial.println("      gamePal_9_DualCore_Fix");
    Serial.println("   ESP32-S3 LIVE PAL");
    Serial.println("======================");
    // VGA
    if (!initVGACore0()) {
        Serial.println("VGA INIT ERROR");
        while (true) delay(100);
    }
    Serial.printf("[GAME] CORE: %d\n", xPortGetCoreID());
    delay(100);
    int isrCore = vga.ISRCore();
    Serial.printf("[VGA] ISR CORE: %d (-1 = not instrumented or not observed)\n", isrCore);
    // BACKGROUND
    Serial.printf( "BG size: %dx%d\n", BG_W, BG_H );
    if (!vga.loadBG(_sunset)){
        Serial.println("BG LOAD ERROR");
        while (true)
            delay(100);
    }
    Serial.println("Background OK");
    // SPRITES
    if (!initSprites()){
        Serial.println("SPRITE ERROR");
        while (true)
            delay(100);
    }
    // TRAFFIC
    randomSeed(micros());
    weatherRng ^= micros();
    if(!weatherRng) weatherRng=1;
    initTraffic();
    prepareRoadTables();
    prepareLighting();
    prepareCityPanorama();
    // MEMORY
    Serial.println();
    Serial.printf( "Free HEAP: %lu bytes\n", (unsigned long)ESP.getFreeHeap() );
    Serial.printf( "Free PSRAM: %lu bytes\n", (unsigned long)ESP.getFreePsram() );
    // START GAME
    location = Location::COAST;
    locationStart = millis();
    dayStart = locationStart;
    gameFrame = 0;
    Serial.println();
    Serial.println("Game started!");
    Serial.println("LOCATION: COAST ROAD");
}
// LOOP
void loop(){
    updateGame();
    renderGame();
    gameFrame++;
}
