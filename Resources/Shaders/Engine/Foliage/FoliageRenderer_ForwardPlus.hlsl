#ifndef _FoliageRenderer_ForwardPlus__
#define _FoliageRenderer_ForwardPlus__

#include "FoliageHelper.hlsl"

struct PassData {
	uint   placementBufferIndex;
    uint   heightMapIndex;
	uint   normalMapIndex;
	uint   rotateToCamera;        // Foliage在渲染时，是否强制朝向摄像机
    float2 worldMeterSize;
    uint   heightScale;
    uint   placementSizePerAxis;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD;
	float3 tangent   : TANGENT0;
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

v2p VSMain(uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) {
	GrassBendSettings bendSettings;
	bendSettings.relTipOffsetZ = 0.7f;
	bendSettings.controlPointZ = bendSettings.relTipOffsetZ * 0.5f;
	bendSettings.controlPointY = 0.8f;

	StructuredBuffer<Placement> culledPlacementList = ResourceDescriptorHeap[PassDataCB.placementBufferIndex];

	Placement blade = culledPlacementList[instanceID];

	float2 uv = GetGrassUV(blade, vertexID, cb.numVertices);
	float2 wind = float2(0.0f, 0.0f);
	float3 position = GetGrassPosition(blade, uv, blade.height, cb.halfWidth, bendSettings, wind);
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

	output.albedoMetalness  = float4(1.0f, 1.0f, 1.0f, 0.0f);
	output.positionEmission = float4(float3(0.0f, input.wsPos.y, 0.0f), 1.0f);
	output.normalRoughness  = float4(input.wsNormal, 1.0f);
	output.motionVector     = float4(velocity, 0.0f);
	output.viewDepth        = input.vsPos.z;

    return output;
}

#endif