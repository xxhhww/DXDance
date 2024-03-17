#ifndef _FarTerrainTextureBaker__
#define _FarTerrainTextureBaker__

#include "TerrainHeader.hlsl"

struct PassData{
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
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
	Texture2DArray terrainAlbedoTextureArray = ResourceDescriptorHeap[PassDataCB.terrainAlbedoTextureArrayIndex];
	Texture2DArray terrainNormalTextureArray = ResourceDescriptorHeap[PassDataCB.terrainNormalTextureArrayIndex];
	Texture2D      terrainTiledSplatMap      = ResourceDescriptorHeap[PassDataCB.terrainSplatMapIndex];
	RWTexture2D<float4> outputAlbedoMap      = ResourceDescriptorHeap[PassDataCB.outputAlbedoMapIndex];
	RWTexture2D<float4> outputNormalMap      = ResourceDescriptorHeap[PassDataCB.outputNormalMapIndex];

	uint2 coord = id.xy;
    if(coord.x < 0 || coord.x >= PassDataCB.vertexCountPerAxis || coord.y < 0 || coord.y >= PassDataCB.vertexCountPerAxis) return;

	// 计算当前纹理像素对应的地形顶点的uvHeight
	float2 tmpPos = float2(coord.x * PassDataCB.vertexSpaceInMeterSize, coord.y * PassDataCB.vertexSpaceInMeterSize);
	float2 uvHeight = tmpPos / PassDataCB.terrainMeterSize;

	// 计算顶点在terrainAlbedoTexture中的偏移
	float2 offsetInTileTexture = tmpPos % PassDataCB.worldMeterSizePerTiledTexture;
	float2 uvTiled = offsetInTileTexture / PassDataCB.worldMeterSizePerTiledTexture;

	// 根据uvHeight采样splatMap
	float4 blend = terrainTiledSplatMap.SampleLevel(SamplerLinearWrap, uvHeight, 0u).rgba;

	float4 rChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 0.0f), 0).rgba, 2.2f);
	float4 rChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 0.0f), 0).rgba;

	float4 gChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 1.0f), 0).rgba, 2.2f);
	float4 gChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 1.0f), 0).rgba;

	float4 bChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 2.0f), 0).rgba, 2.2f);
	float4 bChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 2.0f), 0).rgba;

	float4 aChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 3.0f), 0).rgba, 2.2f);
	float4 aChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(uvTiled, 3.0f), 0).rgba;

	// Blend
	float3 blendAlbedo = blend.r * rChannelAlbedo.rgb + blend.g * gChannelAlbedo.rgb + blend.b * bChannelAlbedo.rgb + blend.a * aChannelAlbedo.rgb;
    float3 blendNormal = blend.r * rChannelNormal.rgb + blend.g * gChannelNormal.rgb + blend.b * bChannelNormal.rgb + blend.a * aChannelNormal.rgb;

	outputAlbedoMap[coord.xy].rgba = float4(blendAlbedo.rgb, 1.0f);
	outputNormalMap[coord.xy].rgba = float4(coord.xy, 0.0f, 0.0f);
}

#endif