#ifndef _UpdateLookUpMap__
#define _UpdateLookUpMap__

#include "RvtHelper.hlsl"

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
	float4 indexInfo : IndexInfo;	// 存储的索引信息
};

struct p2o {
	float4 rvtLookUpMap : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<DrawRvtLookUpMapRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];

	DrawRvtLookUpMapRequest drawRequest = drawRequestBuffer[instanceID];

	float2 pos = saturate(mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix).xy);

	v2p output;
	output.currCsPos = float4(2.0f * pos - 1.0f, 0.5f, 1.0f);
	output.indexInfo = float4(drawRequest.tilePos, drawRequest.mipLevel, 0.0f);

	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	// output.rvtLookUpMap = input.indexInfo;
	output.rvtLookUpMap = float4(GetLODColor(input.indexInfo.z), 1.0f);
	return output;
}

#endif