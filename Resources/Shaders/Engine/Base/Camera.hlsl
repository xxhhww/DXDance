#ifndef _Camera__
#define _Camera__

struct Camera
{
    float4 Position;
    // 16 byte boundary
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 InverseView;
    float4x4 InverseProjection;
    float4x4 InverseViewProjection;
    float4x4 Jitter;
    float4x4 ViewProjectionJitter;
    // 16 byte boundary
    float NearPlane;
    float FarPlane;
    float ExposureValue100;
    float FoVH;
    // 16 byte boundary
    float FoVV;
    float FoVHTan;
    float FoVVTan;
    float AspectRatio; // W/H
    // 16 byte boundary
    float4 Front;
    // 16 byte boundary
    float2 UVJitter;
    uint32_t Pad0__;
    uint32_t Pad1__;
    // 16 byte boundary
    float4 Planes[6];
};

#endif