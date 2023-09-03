#ifndef _UpdateRvtTiledMap__
#define _UpdateRvtTiledMap__

#include "RvtHelper.hlsl"

struct PassData{
	uint32_t terrainHeightMapIndex;
	uint32_t terrainNormalMapIndex;
	uint32_t terrainSplatMapIndex;
	float heightScale;

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

	uint32_t drawRequestBufferIndex;
	float pad1;
	float pad2;
	float pad3;
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
	float4 currCsPos : SV_POSITION;
	float3 wsPos     : POSITION1;
	float2 uv        : TEXCOORD0;
};

struct p2o {
	float4 albedoHeight : SV_TARGET0;
	float4 normal       : SV_TARGET1;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<DrawRvtTiledMapRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];

	DrawRvtTiledMapRequest drawRequest = drawRequestBuffer[instanceID];

	v2p output;
	output.currCsPos = mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix);
	output.wsPos = ...
    output.uv = input.uv;

	return output;
}

p2o PSMain(v2p input) {
	p2o output;

	return output;
}

#endif