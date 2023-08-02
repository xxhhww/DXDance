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

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD0;
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

#endif