#ifndef _UpdateRvtTiledMap__
#define _UpdateRvtTiledMap__

#include "RvtHelper.hlsl"

struct PassData{
	uint32_t drawRequestBufferIndex;
	uint32_t terrainHeightMapIndex;
	uint32_t terrainNormalMapIndex;
	uint32_t terrainSplatMapIndex;

	float2 terrainMeterSize;
	float  terrainHeightScale;
	float  pad;

	uint32_t rChannelAlbedoMapIndex;
	uint32_t rChannelNormalMapIndex;
	uint32_t rChannelRoughnessMapIndex;
	uint32_t rChannelHeightMapIndex;

	uint32_t gChannelAlbedoMapIndex;
	uint32_t gChannelNormalMapIndex;
	uint32_t gChannelRoughnessMapIndex;
	uint32_t gChannelHeightMapIndex;

	uint32_t bChannelAlbedoMapIndex;
	uint32_t bChannelNormalMapIndex;
	uint32_t bChannelRoughnessMapIndex;
	uint32_t bChannelHeightMapIndex;

	uint32_t aChannelAlbedoMapIndex;
	uint32_t aChannelNormalMapIndex;
	uint32_t aChannelRoughnessMapIndex;
	uint32_t aChannelHeightMapIndex;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

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
	float4 blendTile   : Offset0;
	float4 tileOffset  : Offset1;
};

struct p2o {
	float4 albedo : SV_TARGET0;
	float4 normal : SV_TARGET1;
};

float4 SampleTerrainSplatMap(float2 uv) {
	uv.y = 1.0f - uv.y;

	Texture2D<float4> splatMap = ResourceDescriptorHeap[PassDataCB.terrainSplatMapIndex];

	float4 splat = splatMap.SampleLevel(SamplerLinearWrap, uv, 0u).rgba;

	return splat;
}

float4 GetHeightBlend(float high1, float high2, float high3, float high4, float4 splatWeight) {
    float4 blend = float4(high1, high2, high3, high4) * splatWeight;
    float ma = max(blend.r, max(blend.g, max(blend.b, blend.a)));

    //与权重最大的通道进行对比，高度差在_Weight范围内的将会保留,_Weight不可以为0
    blend = max(blend - ma + 0.2f, 0.0f) * splatWeight;
    return blend / (blend.r + blend.g + blend.b + blend.a);
}

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<DrawRvtTiledMapRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];
	DrawRvtTiledMapRequest drawRequest = drawRequestBuffer[instanceID];

	v2p output;
	output.currCsPos = mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix);
	output.uv = input.uv;
	output.blendTile = drawRequest.blendTile;
	output.tileOffset = drawRequest.tileOffset;

	return output;
}

p2o PSMain(v2p input) {
	Texture2D rChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.rChannelAlbedoMapIndex];
	Texture2D rChannelNormalMap = ResourceDescriptorHeap[PassDataCB.rChannelNormalMapIndex];
	Texture2D rChannelHeightMap = ResourceDescriptorHeap[PassDataCB.rChannelHeightMapIndex];

	Texture2D gChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.gChannelAlbedoMapIndex];
	Texture2D gChannelNormalMap = ResourceDescriptorHeap[PassDataCB.gChannelNormalMapIndex];
	Texture2D gChannelHeightMap = ResourceDescriptorHeap[PassDataCB.gChannelHeightMapIndex];

	Texture2D bChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.bChannelAlbedoMapIndex];
	Texture2D bChannelNormalMap = ResourceDescriptorHeap[PassDataCB.bChannelNormalMapIndex];
	Texture2D bChannelHeightMap = ResourceDescriptorHeap[PassDataCB.bChannelHeightMapIndex];

	Texture2D aChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.aChannelAlbedoMapIndex];
	Texture2D aChannelNormalMap = ResourceDescriptorHeap[PassDataCB.aChannelNormalMapIndex];
	Texture2D aChannelHeightMap = ResourceDescriptorHeap[PassDataCB.aChannelHeightMapIndex];

	float2 blendUV = input.uv * input.blendTile.xy + input.blendTile.zw;
	float4 blend = SampleTerrainSplatMap(blendUV);

	float4 finalAlbedo = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 finalNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Height

    float2 transUv = input.uv * input.tileOffset.xy + input.tileOffset.zw;
    float4 rChannelAlbedo = pow(rChannelAlbedoMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba, 2.2f);
    float4 rChannelNormal = rChannelNormalMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba;

    transUv = input.uv * input.tileOffset.xy + input.tileOffset.zw;
    float4 gChannelAlbedo = pow(gChannelAlbedoMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba, 2.2f);
    float4 gChannelNormal = gChannelNormalMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba;

    
	transUv = input.uv * input.tileOffset.xy + input.tileOffset.zw;
    float4 bChannelAlbedo = pow(bChannelAlbedoMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba, 2.2f);
    float4 bChannelNormal = bChannelNormalMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba;

    
	transUv = input.uv * input.tileOffset.xy + input.tileOffset.zw;
    float4 aChannelAlbedo = pow(aChannelAlbedoMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba, 2.2f);
    float4 aChannelNormal = aChannelNormalMap.SampleLevel(SamplerLinearWrap, transUv, 0).rgba;

	// Blend
	finalAlbedo = blend.r * rChannelAlbedo + blend.g * gChannelAlbedo + blend.b * bChannelAlbedo + blend.a * aChannelAlbedo;
    finalNormal = blend.r * rChannelNormal + blend.g * gChannelNormal + blend.b * bChannelNormal + blend.a * aChannelNormal;

	p2o output;
	output.albedo.rgba = finalAlbedo;
	output.normal.xyzw = finalNormal;
	return output;
}

#endif