#pragma once
#include <Arduino.h>
#include <Sprite.h>
struct CachedSprite { Sprite* sprite=nullptr; int w=0,h=0; };
struct TrafficCar { float z,lane,speed; uint8_t color; bool passed; };
struct RoadPoint { int y,center,halfWidth,depth; };
