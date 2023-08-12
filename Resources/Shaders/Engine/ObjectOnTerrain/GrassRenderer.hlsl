#ifndef _GrassRenderer__
#define _GrassRenderer__

#include "ObjectOnTerrainHelper.hlsl"

struct PassData {
	uint  grassPlacementBufferIndex;
	uint  numVertices;	// LOD
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
    float4 userData  : USERDATA;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD0;
};

/*
struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD0;
	float3 tangent   : TANGENT0;
};
*/

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<GrassBlade> grassBladeBuffer = ResourceDescriptorHeap[PassDataCB.grassPlacementBufferIndex];

	GrassBlade grassBlade = grassBladeBuffer[instanceID];

    // 构建能将Foliage强制朝向摄像机的旋转矩阵，而非从模型变换矩阵中进行提取

    v2p output;
    float4 tempWsPos = float4(input.lsPos + grassBlade.position.xyz, 1.0f);

    // TODO 对Foliage施加风力的影响

    // 目前假设Foliage的世界坐标不发生改变(实际上模型变换中的旋转部分会随着摄像机的旋转而变换)
	float3 currWsPos = tempWsPos.xyz;
	float3 prevWsPos = tempWsPos.xyz;

    float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

    output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;

    float3 tempNormal = float3(0.0f, 1.0f, 0.0f);
    output.wsNormal = normalize(tempNormal.xyz);
	output.uv = input.uv;

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

	output.albedoMetalness  = float4(1.0f, 1.0f, 1.0f, 0.0f);
	output.positionEmission = float4(float3(0.0f, input.wsPos.y, 0.0f), 1.0f);
	output.normalRoughness  = float4(input.wsNormal, 1.0f);
	output.motionVector     = float4(velocity, 0.0f);
	output.viewDepth        = input.vsPos.z;

    return output;
}

/*
v2p VSMain(uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) {
	GrassBendSettings bendSettings;
	bendSettings.relTipOffsetZ = 0.7f;
	bendSettings.controlPointZ = bendSettings.relTipOffsetZ * 0.5f;
	bendSettings.controlPointY = 0.8f;

	StructuredBuffer<Placement> culledPlacementList = ResourceDescriptorHeap[PassDataCB.grassPlacementBufferIndex];

	Placement blade = culledPlacementList[instanceID];

	float halfWidth = 0.05f * 0.5f;

	float2 uv = GetGrassUV(blade, vertexID, PassDataCB.numVertices);
	float2 wind = float2(0.0f, 0.0f);
	float3 position = GetGrassPosition(blade, uv, blade.height, halfWidth, bendSettings, wind);
	float3 normal = GetGrassNormal(blade, uv, blade.height, bendSettings, wind);

	v2p output;
	output.uv = uv;
	output.wsNormal = normal;
	output.tangent = float3(blade.facing.y, 0.f, blade.facing.x);
	output.wsPos = position;
	output.currCsPos = mul(float4(position, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	output.prevCsPos = mul(float4(position, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);
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
	output.shadingResult   = float4(0.5f, 0.9f, 0.1f, 1.0f);
	output.normalRoughness = float4(input.wsNormal, 0.99f);
	output.screenVelocity  = float2(velocity.xy);

    return output;
}
*/

#endif