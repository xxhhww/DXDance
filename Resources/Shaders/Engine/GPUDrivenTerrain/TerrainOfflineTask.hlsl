#ifndef _MinMaxHeightMapGenerator__
#define _MinMaxHeightMapGenerator__

struct PassData {
	uint heightMapIndex;
	uint minMaxHeightMapIndex;
	float patchTexScale; // MostDetailedPatch网格(1280 * 1280)相对于当前HeightMap的缩放
	uint inMipMapIndex;
    uint outMipMapIndex;
    uint normalMapIndex; // From HeightMap
    uint heightMapWidth;
    uint heightMapHeight;
};

#define PassDataType PassData

#include "../MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void GenMinMaxHeightMap (uint3 id : SV_DispatchThreadID) {
    Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];
    RWTexture2D<float4> minMaxHeightMap = ResourceDescriptorHeap[PassDataCB.minMaxHeightMapIndex];

    float2 samplePoint = id.xy * PassDataCB.patchTexScale;
    float sampleInterval = 1.0f * PassDataCB.patchTexScale;

    float h1 = heightMap[samplePoint.xy].r;
    float h2 = heightMap[samplePoint.xy + float2(sampleInterval, 0.0f)].r;
    float h3 = heightMap[samplePoint.xy + float2(0.0f, sampleInterval)].r;
    float h4 = heightMap[samplePoint.xy + float2(sampleInterval, sampleInterval)].r;

    float hmin = min(min(h1, h2), min(h3, h4));
    float hmax = max(max(h1, h2), max(h3, h4));
    minMaxHeightMap[id.xy].rgba = float4(hmin, hmax, 0.0f, 1.0f);
}

[numthreads(5, 5, 1)]
void GenMinMaxHeightMapMipMap (uint3 id : SV_DispatchThreadID) {
    Texture2D<float4> inMipMap = ResourceDescriptorHeap[PassDataCB.inMipMapIndex];
    RWTexture2D<float4> outMipMap = ResourceDescriptorHeap[PassDataCB.outMipMapIndex];

    uint2 inLoc = id.xy * 2u;
    float2 h1 = inMipMap[inLoc].rg;
    float2 h2 = inMipMap[inLoc + uint2(1u, 0u)].rg;
    float2 h3 = inMipMap[inLoc + uint2(0u, 1u)].rg;
    float2 h4 = inMipMap[inLoc + uint2(1u, 1u)].rg;

    float hmin = min(min(h1.r, h2.r), min(h3.r, h4.r));
    float hmax = max(max(h1.g, h2.g), max(h3.g, h4.g));
    outMipMap[id.xy] = float4(hmin, hmax, 0u, 1u);
}

float3 GetPosition(uint2 coord) {
    Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];
    uint w = PassDataCB.heightMapWidth;
    uint h = PassDataCB.heightMapHeight;
    float height = heightMap[coord].r;
    return float3(coord.x * 1.0f / w, height, coord.y * 1.0f / h); 
}

float3 GetNormal(float3 v1, float3 v2) {
    return normalize(-cross(v1, v2)).xyz;
}

[numthreads(8, 8, 1)]
void GenNormalMap(uint3 id : SV_DispatchThreadID) {
    Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];
    RWTexture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

    uint2 coord = id.xy;

    float3 p0 = GetPosition(coord);
    float3 v1 = GetPosition(coord + int2(1, 0)) - p0;
    float3 v2 = GetPosition(coord + int2(0, 1)) - p0;
    float3 v3 = GetPosition(coord + int2(-1, 0)) - p0;
    float3 v4 = GetPosition(coord + int2(0, -1)) - p0;

    float3 normal = GetNormal(v1, v2) + GetNormal(v2, v3) + GetNormal(v3, v4) + GetNormal(v4, v1);
    normal = normalize(normal);
    normal = (normal + 1) * 0.5;
    normalMap[coord] = float4(normal.xzy, 1);
}

#endif