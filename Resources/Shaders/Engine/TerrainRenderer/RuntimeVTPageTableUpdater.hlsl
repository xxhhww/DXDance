#ifndef _TerrainRvtLookupUpdater__
#define _TerrainRvtLookupUpdater__

#include "TerrainHeader.hlsl"

struct PassData{
	uint  drawRequestBufferIndex;
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
	float4 color     : COLOR;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	uint4  indexData : INDEXDATA;
};

struct p2o {
	uint4  indexData : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<GpuUpdateRuntimeVTPageTableRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];

	GpuUpdateRuntimeVTPageTableRequest drawRequest = drawRequestBuffer[instanceID];

	float2 pos = saturate(mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix).xy);

	v2p output;
	output.currCsPos = float4(2.0f * pos - 1.0f, 0.5f, 1.0f);
	output.indexData = uint4(drawRequest.tilePosX, drawRequest.tilePosY, drawRequest.pageLevel, 1u);

	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	output.indexData = input.indexData;
	return output;
}

#endif