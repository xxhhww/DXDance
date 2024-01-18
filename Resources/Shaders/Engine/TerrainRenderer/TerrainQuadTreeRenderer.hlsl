#ifndef _TerrainQuadTreeRenderer__
#define _TerrainQuadTreeRenderer__

#include "TerrainHeader.hlsl"

struct PassData {
	float2 terrainMeterSize;
	float  terrainHeightScale;
	uint   culledPatchListIndex;

	uint terrainTextureArrayIndex;
	uint terrainHeightMapAtlasIndex;
	uint terrainAlbedoMapAtlasIndex;
	uint terrainNormalMapAtlasIndex;

	// x: page table size
	// y: virtual texture size
	// z: max mipmap level
	// w: mipmap level bias
	float4 vtFeedbackParams;
	float4 vtRealRect;
	// x: padding size
	// y: tileSize
	// z: physical texture size x
	// w: physical texture size y
	float4 vtPhysicalMapParams;

	uint  pageTableMapIndex;
	uint  physicalAlbedoMapIndex;
	uint  physicalNormalMapIndex;
	float pad1;

	uint  lodDebug;
	float pad2;
	float pad3;
	float pad4;
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
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
	// float2 uvHeight  : TEXCOORD0;
	// float2 uvVT      : TEXCOORD1;
	float2 uv        : TEXCOORD2;
	uint   lod       : LOD;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3; 
	float  viewDepth        : SV_TARGET4;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];
	Texture2D terrainHeightMapAtlas = ResourceDescriptorHeap[PassDataCB.terrainHeightMapAtlasIndex];

	v2p output;

	RenderPatch patch = culledPatchList[instanceID];
	uint  lod = patch.lod;
	float scale = pow(2, lod);
	input.lsPos.xz *= scale;
	input.lsPos.xz += patch.position;

	// 地形位置不会变化，因此currWsPos与prevWsPos是一样的
	float3 currWsPos = input.lsPos;
	float3 prevWsPos = input.lsPos;

	float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

	output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;
	// output.uvHeight = heightUV;
	// output.uvVT = (currWsPos.xz - PassDataCB.vtRealRect.xy) / PassDataCB.vtRealRect.zw;
	output.uv = input.uv;
	output.lod = lod;
	return output;	
}

p2o PSMain(v2p input) {
	float3 currLodColor = GetLODColor(input.lod);

	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	float3 albedo = currLodColor;
	float3 normal = float3(0.0f, 1.0f, 0.0f);

	p2o output;
	output.albedoMetalness  = float4(albedo, 0.0f);
	output.positionEmission = float4(input.wsPos, 0.0f);
	output.normalRoughness  = float4(normal, 1.0f);
	output.motionVector     = float4(velocity.xy, 0.0f, 0.0f);
	output.viewDepth        = input.vsPos.z;

	return output;
}

#endif