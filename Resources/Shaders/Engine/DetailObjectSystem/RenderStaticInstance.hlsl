#ifndef _RenderStaticInstance__
#define _RenderStaticInstance__

#include "StaticInstanceHelper.hlsl"
#include "../Base/Light.hlsl"

struct PassData {
	uint instanceAlbedoMapIndex;
	uint instanceNormalMapIndex;
	uint instanceRoughnessMapIndex;
	uint instanceAoMapIndex;	

	uint transformsBufferIndex;
	uint visibleInstanceIndexBufferIndex;	// 存放的是Instance的直接索引
	float pad1;
	float pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float4 color     : COLOR;
};

struct v2p {
	float4 currCsPos  : SV_POSITION;
	float4 prevCsPos  : POSITION1;
	float3 wsPos      : POSITION2;
	float2 uv         : TEXCOORD2;
	float3 debugColor : DebugColor;
};

struct p2o {
	float4 shadingResult	: SV_TARGET0;
	float4 normalRoughness	: SV_TARGET1;
	float2 screenVelocity	: SV_TARGET2;
};

v2p VSMain(a2v input, uint vertexID : SV_VERTEXID, uint instanceID : SV_InstanceID) {
	StructuredBuffer<float4x4> transformsBuffer = ResourceDescriptorHeap[PassDataCB.transformsBufferIndex];
	StructuredBuffer<uint> visibleInstanceIndexBuffer = ResourceDescriptorHeap[PassDataCB.visibleInstanceIndexBufferIndex];

	uint directIndex = visibleInstanceIndexBuffer[instanceID];
	float4x4 instanceTransform = transformsBuffer[directIndex];

	// 静态实例化网格
	v2p output;
	float3 currWsPos = mul(float4(input.lsPos, 1.0f), instanceTransform).xyz;
	float3 prevWsPos = currCsPos;

	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

	output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.uv = input.uv;

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
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	p2o output;
	output.shadingResult   = float4(input.debugColor, 1.0f);
	output.normalRoughness = float4(input.wsNormal, 1.0f);
	output.screenVelocity  = float2(velocity.xy);

	return output;
}

#endif