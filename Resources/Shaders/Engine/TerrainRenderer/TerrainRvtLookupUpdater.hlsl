#ifndef _TerrainRvtLookupUpdater__
#define _TerrainRvtLookupUpdater__

#include "TerrainHeader.hlsl"

struct PassData{
	uint drawRequestBufferIndex;
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
	float4 color     : COLOR;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 indexData : INDEXDATA;
};

struct p2o {
	float4 indexData : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<GpuUpdateTerrainRvtLookupRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];

	DrawRvtLookUpMapRequest drawRequest = drawRequestBuffer[instanceID];

	float2 pos = saturate(mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix).xy);

	v2p output;
	output.currCsPos = float4(2.0f * pos - 1.0f, 0.5f, 1.0f);
	output.indexData = float4(drawRequest.tilePos, drawRequest.mipLevel, 1.0f);

	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	output.indexData = input.indexData;
	return output;
}

#endif