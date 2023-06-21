#ifndef _Utils__
#define _Utils__

// v is a unit vector. The result is an octahedral vector on the [-1, +1] square.
float2 OctEncode(float3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    float2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * sign(result.xy);
    }
    return result;
}

// Returns a unit vector. Argument o is an octahedral vector on the [-1, +1] square
float3 OctDecode(float2 o) {
    float3 v = float3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * sign(v.xy);
    }
    return normalize(v);
}

float2 TexelIndexToUV(uint2 index, uint2 textureSize) {
    return float2(index + 0.5) / textureSize;
}



#endif