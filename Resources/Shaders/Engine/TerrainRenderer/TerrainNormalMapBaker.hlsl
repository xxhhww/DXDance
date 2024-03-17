#ifndef _TerrainNormalMapBaker__
#define _TerrainNormalMapBaker__

#include "TerrainHeader.hlsl"

struct PassData {
	uint  terrainHeightMapIndex;
	uint  terrainNormalMapIndex;
	uint  terrainSplatMapIndex;
	float worldMeterSizePerTiledTexture;

	uint  terrainAlbedoTextureArrayIndex;
	uint  terrainNormalTextureArrayIndex;
	uint  outputAlbedoMapIndex;
	uint  outputNormalMapIndex;

	float4x4 mvpMatrix;

	float4 tileOffset;
	float4 blendOffset;

    uint  vertexCountPerAxis;
    float vertexSpaceInMeterSize;   // 地形两个顶点之间的间隔
    float terrainMeterSize;
    float terrainHeightScale;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

float3 GetPosition(uint2 coord) {
    Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];
    float height = heightMap[coord].r;

    // 转换到世界空间
    float3 worldPos = float3(coord.x * PassDataCB.vertexSpaceInMeterSize, height * PassDataCB.terrainHeightScale, (PassDataCB.vertexCountPerAxis - 1 - coord.y) * PassDataCB.vertexSpaceInMeterSize);
    worldPos.x -= (PassDataCB.terrainMeterSize / 2.0f);
    worldPos.z -= (PassDataCB.terrainMeterSize / 2.0f);

    return worldPos;
}

float3 GetNormal(float3 v1, float3 v2) {
    return normalize(-cross(v1, v2)).xyz;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];
    RWTexture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.terrainNormalMapIndex];

    uint2 coord = id.xy;
    if(coord.x < 0 || coord.x >= PassDataCB.vertexCountPerAxis || coord.y < 0 || coord.y >= PassDataCB.vertexCountPerAxis) return;

    float4 h;
    h[0] = heightMap[coord + uint2(0, -1)].r * PassDataCB.terrainHeightScale;
    h[1] = heightMap[coord + uint2(-1, 0)].r * PassDataCB.terrainHeightScale;
    h[2] = heightMap[coord + uint2(1,  0)].r * PassDataCB.terrainHeightScale;
    h[3] = heightMap[coord + uint2(0,  1)].r * PassDataCB.terrainHeightScale;
    
    float3 n;
    n.z = h[3] - h[0];
    n.x = h[1] - h[2];
    n.y = 2;

    n = normalize(n);
    n = (n + 1.0f) * 0.5f;
    normalMap[coord] = float4(n.xyz, 1.0f);
    /*
    float3 p0 = GetPosition(coord);
    float3 v1 = GetPosition(coord + int2(1, 0)) - p0;
    float3 v2 = GetPosition(coord + int2(0, 1)) - p0;
    float3 v3 = GetPosition(coord + int2(-1, 0)) - p0;
    float3 v4 = GetPosition(coord + int2(0, -1)) - p0;

    float3 normal = GetNormal(v1, v2) + GetNormal(v2, v3) + GetNormal(v3, v4) + GetNormal(v4, v1);
    normal = normalize(normal);
    normal = (normal + 1) * 0.5;
    normalMap[coord] = float4(normal.xyz, 1.0f);
    */
}

#endif