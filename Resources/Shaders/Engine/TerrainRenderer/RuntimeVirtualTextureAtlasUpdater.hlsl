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
	StructuredBuffer<GpuDrawRuntimeVirtualTextureAtlasRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];
	GpuDrawRuntimeVirtualTextureAtlasRequest drawRequest = drawRequestBuffer[instanceID];

	v2p output;
	output.currCsPos = mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix);
	output.uv = input.uv;
	output.tileOffset = drawRequest.tileOffset;
	output.blendOffset = drawRequest.blendOffset;

	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	output.albedo.rgba = float4(0.5f, 0.5f, 0.5f, 1.0f);
	output.normal.rgba = float4(0.5f, 0.5f, 0.5f, 1.0f);
	return output;
}


#endif