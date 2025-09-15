#pragma once
#include <cmath>  // для sqrt, sin, cos и т.д.

////////////////////////////////////////
// 2D VECTOR
////////////////////////////////////////
struct vec2d {
    float x, y;

    vec2d() : x(0), y(0) {}
    vec2d(float value) : x(value), y(value) {}
    vec2d(float _x, float _y) : x(_x), y(_y) {}

    // OPERATORS
    vec2d operator+(vec2d const& other) const { return vec2d(x + other.x, y + other.y); }
    vec2d operator-(vec2d const& other) const { return vec2d(x - other.x, y - other.y); }
    vec2d operator*(vec2d const& other) const { return vec2d(x * other.x, y * other.y); }
    vec2d operator/(vec2d const& other) const { return vec2d(x / other.x, y / other.y); }
    vec2d operator-() const { return vec2d(-x, -y); }

    // SCALAR OPERATIONS
    vec2d operator*(float s) const { return vec2d(x*s, y*s); }
    vec2d operator/(float s) const { return vec2d(x/s, y/s); }

    // FUNCTIONS
    float length() const { return std::sqrt(x*x + y*y); }
    vec2d normalized() const { float len = length(); return len ? vec2d(x/len, y/len) : vec2d(0,0); }
    float dot(vec2d const& other) const { return x*other.x + y*other.y; }
};

////////////////////////////////////////
// 3D VECTOR
////////////////////////////////////////
struct vec3d {
    float x, y, z;

    vec3d() : x(0), y(0), z(0) {}
    vec3d(float value) : x(value), y(value), z(value) {}
    vec3d(float _x, vec2d const& v) : x(_x), y(v.x), z(v.y) {}
    vec3d(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // OPERATORS
    vec3d operator+(vec3d const& other) const { return vec3d(x + other.x, y + other.y, z + other.z); }
    vec3d operator-(vec3d const& other) const { return vec3d(x - other.x, y - other.y, z - other.z); }
    vec3d operator*(vec3d const& other) const { return vec3d(x * other.x, y * other.y, z * other.z); }
    vec3d operator/(vec3d const& other) const { return vec3d(x / other.x, y / other.y, z / other.z); }
    vec3d operator-() const { return vec3d(-x, -y, -z); }

    // SCALAR OPERATIONS
    vec3d operator*(float s) const { return vec3d(x*s, y*s, z*s); }
    vec3d operator/(float s) const { return vec3d(x/s, y/s, z/s); }

    // FUNCTIONS
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    vec3d normalized() const { float len = length(); return len ? vec3d(x/len, y/len, z/len) : vec3d(0,0,0); }
    float dot(vec3d const& other) const { return x*other.x + y*other.y + z*other.z; }

    // CROSS PRODUCT
    vec3d cross(vec3d const& other) const {
        return vec3d(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
};

/*
    static int t = 0; // сохраняем кадр
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            float x = (float)i / width * 2.0f - 1.0f;
            float y = (float)j / height * 2.0f - 1.0f;
            x *= aspect;
            x += sin(t * 0.01f); // ускорили движение
            if (x*x + y*y < 0.5f) {
                gfx.putPixel(i, j, 255);
            } else {
                gfx.putPixel(i, j, 0);
            }
        }
    }
    t++; // следующий кадр
*/