#ifndef _MathCommon__
#define _MathCommon__

static const float PI = 3.1415926535897932384626433832795;

float  Square(float v)  { return v * v; }
float2 Square(float2 v) { return float2(v.x * v.x, v.y * v.y); }
float3 Square(float3 v) { return float3(v.x * v.x, v.y * v.y, v.z * v.z); }
float4 Square(float4 v) { return float4(v.x * v.x, v.y * v.y, v.z * v.z, v.w * v.w); }

#endif