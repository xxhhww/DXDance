#ifndef _OpaquePass__
#define _OpaquePass__

struct PassData {
	uint index1;
	uint index2;
	uint index3;
	uint index4;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

cbuffer ItemData : register(b0, space0) 
{
	float4x4 prevModelTrans;	// 前一帧的世界变换矩阵
	float4x4 currModelTrans;    // 当前帧的世界变换矩阵
    float4 center;				// boundingBox
    float4 extend;
};

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
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
	float2 uv        : TEXCOORD2;
	float3 wsNormal  : NORMAL0;
	float3 debugColor: COLOR0;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3; 
	float  viewDepth        : SV_TARGET4;
};

v2p VSMain(a2v input, uint vertexID : SV_VERTEXID) {
	v2p output;

	float3 currWsPos = mul(float4(input.lsPos, 1.0f), currModelTrans).xyz;
	float3 prevWsPos = mul(float4(input.lsPos, 1.0f), prevModelTrans).xyz;

	float3 wsNormal = mul(float4(input.lsNormal, 0.0f), currModelTrans).xyz;

	float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

	output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;
	output.uv = input.uv;
	output.wsNormal = wsNormal;
	if(vertexID % 3 == 0) {
		output.debugColor = float3(1.0f, 0.0f, 0.0f);
	}
	else if(vertexID % 3 == 1) {
		output.debugColor = float3(0.0f, 1.0f, 0.0f);
	}
	else if(vertexID % 3 == 2) {
		output.debugColor = float3(0.0f, 0.0f, 1.0f);
	}

	return output;
}

p2o PSMain(v2p input) {
	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	p2o output;
	output.albedoMetalness  = float4(input.debugColor, 0.0f);
	output.positionEmission = float4(input.wsPos, 0.0f);
	output.normalRoughness  = float4(input.wsNormal, 1.0f);
	output.motionVector     = float4(velocity.xy, 0.0f, 0.0f);
	output.viewDepth        = input.vsPos.z;

	return output;
}

#endif