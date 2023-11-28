#ifndef _CascadeShadowPass__
#define _CascadeShadowPass__

struct PassData {
	float4x4 vpMatrix;	// 转换到光源摄像机裁剪空间的vp矩阵
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

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
};

v2p VSMain(a2v input, uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) {
	v2p output;

	float3 currWsPos = mul(float4(input.lsPos, 1.0f), ItemDataCB.currModelTrans).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), PassDataCB.vpMatrix);

	output.currCsPos = currCsPos;

	return output;
}

void PSMain(v2p input) {
	return;
}

#endif