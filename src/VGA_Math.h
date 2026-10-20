#pragma once

static constexpr int   FP_SHIFT     = 16;
static constexpr int   FP_ONE       = 1 << FP_SHIFT;
static constexpr int   HALF         = 1 << (FP_SHIFT - 1);
static constexpr float deg_to_rad   = 0.01745329251994329577f; // π / 180
static inline    float angle(float deg)         { return (fmodf(deg, 360.0f) * deg_to_rad);}
static constexpr float percentTo(uint16_t p)    { return p * 0.01f; }