#pragma once

namespace Math {

    inline float Mix(float x, float y, float a) { return x * (1.0f - a) + y * a; }
    inline float Max(float a, float b) { return a > b ? a : b; }
    inline float Min(float a, float b) { return a < b ? a : b; }
    inline float Clamp(float v, float a, float b) { return Min(Max(v, a), b); }

}