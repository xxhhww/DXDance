#ifndef _TerrainFeedback__
#define _TerrainFeedback__

#include "TerrainHelper.hlsl"

struct PassData {
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

	float2 worldSize;
	uint heightScale;
	uint culledPatchListIndex;

	uint heightMapIndex;
	uint normalMapIndex;
	uint lodDebug;
	float pad1;
}

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
	float2 uvHeight  : TEXCOORD0;
	float2 uvVT      : TEXCOORD1;
	float2 uv        : TEXCOORD2;
	uint   lod       : LOD;
};

struct p2o {
	uint4  terrainFeedback	: SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];
	Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];

	v2p output;

	RenderPatch patch = culledPatchList[instanceID];
	uint lod = patch.lod;
	float scale = pow(2, lod);
	input.lsPos.xz *= scale;
	input.lsPos.xz += patch.position;

	float2 heightUV = (input.lsPos.xz + (PassDataCB.worldSize * 0.5f) + 0.5f) / (PassDataCB.worldSize + 1.0f);
	heightUV *= 1.0f;
	float height = heightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r;
	input.lsPos.y = height * PassDataCB.heightScale;

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
	output.uvHeight = heightUV;
	output.uvVT = (currWsPos.xz - PassDataCB.vtRealRect.xy) / PassDataCB.vtRealRect.zw;
	output.uv = input.uv;
	output.lod = lod;
	return output;
}

p2o PSMain(v2p input) {
	// Calcute Feedback
	uint2 page = floor(input.uvVT * PassDataCB.vtFeedbackParams.x);
	
	float2 uv = input.uvVT * PassDataCB.vtFeedbackParams.y;
	float2 dx = ddx(uv);
	float2 dy = ddy(uv);
	int mip = clamp(int(0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + PassDataCB.vtFeedbackParams.w), 0, PassDataCB.vtFeedbackParams.z);

	// 是否越界
	uint overBound = (input.uvVT.x > 1.0f || input.uvVT.y > 1.0f) ? 0u : 1u;

	p2o output;
	output.terrainFeedback = uint4(page, mip, overBound);

	return output;
}

#endif