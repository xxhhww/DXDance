#ifndef _RuntimeVirtualTextureAtlasUpdater__
#define _RuntimeVirtualTextureAtlasUpdater__

#include "TerrainHeader.hlsl"

struct PassData{
	uint drawRequestBufferIndex;
	uint terrainSplatMapIndex;
	uint terrainAlbedoTextureArrayIndex;
	uint terrainNormalTextureArrayIndex;

	uint terrainRoughnessTextureArrayIndex;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p {
	float4 currCsPos   : SV_POSITION;
	float2 uv          : TEXCOORD0;
	float4 blendOffset : Offset0;
	float4 tileOffset  : Offset1;
};

struct p2o {
	float4 albedo : SV_TARGET0;
	float4 normal : SV_TARGET1;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<GpuUpdateRuntimeVTAtlasRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];
	GpuUpdateRuntimeVTAtlasRequest drawRequest = drawRequestBuffer[instanceID];

	v2p output;
	output.currCsPos = mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix);
	output.currCsPos.y = - output.currCsPos.y;
	output.uv = input.uv;
	output.tileOffset = drawRequest.tileOffset;
	output.blendOffset = drawRequest.blendOffset;

	return output;
}

p2o PSMain(v2p input) {
	Texture2DArray terrainAlbedoTextureArray = ResourceDescriptorHeap[PassDataCB.terrainAlbedoTextureArrayIndex];
	Texture2DArray terrainNormalTextureArray = ResourceDescriptorHeap[PassDataCB.terrainNormalTextureArrayIndex];
	Texture2D      terrainTiledSplatMap      = ResourceDescriptorHeap[PassDataCB.terrainSplatMapIndex];

	float2 blendUV = input.uv * input.blendOffset.xy + input.blendOffset.zw;
	float2 transUv = input.uv * input.tileOffset.xy + input.tileOffset.zw;

	float4 blend   = terrainTiledSplatMap.SampleLevel(SamplerLinearWrap, blendUV, 0u).rgba;

	float4 rChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 0.0f), 0).rgba, 2.2f);
	float4 rChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 0.0f), 0).rgba;

	float4 gChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 1.0f), 0).rgba, 2.2f);
	float4 gChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 1.0f), 0).rgba;

	float4 bChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 2.0f), 0).rgba, 2.2f);
	float4 bChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 2.0f), 0).rgba;

	float4 aChannelAlbedo = pow(terrainAlbedoTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 3.0f), 0).rgba, 2.2f);
	float4 aChannelNormal = terrainNormalTextureArray.SampleLevel(SamplerLinearWrap, float3(transUv, 3.0f), 0).rgba;

	// Blend
	float3 blendAlbedo = blend.r * rChannelAlbedo.rgb + blend.g * gChannelAlbedo.rgb + blend.b * bChannelAlbedo.rgb + blend.a * aChannelAlbedo.rgb;
    float3 blendNormal = blend.r * rChannelNormal.rgb + blend.g * gChannelNormal.rgb + blend.b * bChannelNormal.rgb + blend.a * aChannelNormal.rgb;

	p2o output;
	output.albedo.rgba = float4(blendAlbedo, 1.0f);
	output.normal.rgba = float4(blendNormal, 1.0f);
	return output;
}


#endif