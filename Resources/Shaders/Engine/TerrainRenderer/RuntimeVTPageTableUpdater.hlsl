#ifndef _TerrainRvtLookupUpdater__
#define _TerrainRvtLookupUpdater__

#include "TerrainHeader.hlsl"

struct PassData{
	uint  updateRequestBufferIndex;
	uint  runtimeVTPageTableCopyIndex;
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
	float2 uv : TEXCOORD;
};

struct p2o {
	uint4  indexData : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<GpuUpdateRuntimeVTPageTableRequest> updateRequestBuffer = ResourceDescriptorHeap[PassDataCB.updateRequestBufferIndex];

	GpuUpdateRuntimeVTPageTableRequest updateRequest = updateRequestBuffer[instanceID];

	float2 pos = saturate(mul(float4(input.lsPos, 1.0f), updateRequest.mvpMatrix).xy);
	pos.y = 1 - pos.y;

	v2p output;
	output.currCsPos = float4(2.0f * pos - 1.0f, 0.5f, 1.0f);
	output.indexData = uint4(updateRequest.rectInPage0Level.x, updateRequest.rectInPage0Level.y, updateRequest.pageLevel, updateRequest.rectInPage0Level.z);
	output.uv = input.uv;
	return output;
}

p2o PSMain(v2p input) {
	RWTexture2D<uint4> runtimeVTPageTableCopy = ResourceDescriptorHeap[PassDataCB.runtimeVTPageTableCopyIndex];

	uint2 localIndex  = input.uv * input.indexData.w;
	uint2 globalIndex = input.indexData.xy + localIndex;

	uint4 copyData = runtimeVTPageTableCopy[globalIndex.xy].rgba;

	if(copyData.z <= input.indexData.z) discard;

	runtimeVTPageTableCopy[globalIndex.xy].z = input.indexData.z;

	p2o output;
	output.indexData = input.indexData;

	return output;
}

#endif