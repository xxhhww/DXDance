#ifndef _MathCommon__
#define _MathCommon__

// Its definition is: one candela is the luminous intensity, in a given direction, of a source
// that emits monochromatic radiation at a frequency of 540THz(i.e a wavelength of 555nm) and whose
// radiant intensity in that direction is 1 / 683 watts per steradian., meaning that Km = 683.
static const float StandardLuminousEfficacy = 683.0f;

static const float PI = 3.1415926535897932384626433832795;

float  Square(float v)  { return v * v; }
float2 Square(float2 v) { return float2(v.x * v.x, v.y * v.y); }
float3 Square(float3 v) { return float3(v.x * v.x, v.y * v.y, v.z * v.z); }
float4 Square(float4 v) { return float4(v.x * v.x, v.y * v.y, v.z * v.z, v.w * v.w); }

#endif