#ifndef _Camera__
#define _Camera__

struct Camera {
    float4 Position;
    float4 LookUp;
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

float LinearizeDepth(float hyperbolicDepth, Camera camera) {
    float n = camera.NearPlane;
    float f = camera.FarPlane;

    return n * f / (f + hyperbolicDepth * (n - f));
}

float HyperbolizeDepth(float linearDepth, Camera camera) {
    float n = camera.NearPlane;
    float f = camera.FarPlane;

    return (((n * f) / linearDepth) - f) / (n - f);
}

float3 NDCDepthToViewPosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 ssuv, // Normalized screen-space (texture) coordinates [0; 1]
    Camera camera) {
    // Have to invert Y due to DirectX convention for [0, 0] to be at the TOP left
    float2 uv = float2(ssuv.x, 1.0 - ssuv.y);

    float z = hyperbolicDepth;
    float2 xy = uv * 2.0 - 1.0;

    float4 ndcPosition = float4(xy, z, 1.0);
    float4 viewSpacePosition = mul(ndcPosition, camera.InverseProjection);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

float3 NDCDepthToWorldPosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 ssuv, // Normalized screen-space (texture) coordinates [0; 1]
    Camera camera) {
    float4 viewSpacePosition = float4(NDCDepthToViewPosition(hyperbolicDepth, ssuv, camera), 1.0f);
    float4 worldSpacePosition = mul(viewSpacePosition, camera.InverseView);

    return worldSpacePosition.xyz;
}

float3 ViewDepthToWorldPosition(float viewDepth, float2 ssuv, Camera camera) {
    float hyperbolicDepth = HyperbolizeDepth(viewDepth, camera);
    return NDCDepthToWorldPosition(hyperbolicDepth, ssuv, camera);
}

#endif