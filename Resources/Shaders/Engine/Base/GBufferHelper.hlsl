#ifndef _GBufferHelper__
#define _GBufferHelper__

struct GBufferSurface {
    float3 albedo;
    float3 position;    // world space position
    float3 normal;      // world space normal
    float roughness;
    float metalness;
};

#endif