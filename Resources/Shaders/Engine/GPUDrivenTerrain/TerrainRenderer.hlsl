#ifndef _TerrainRenderer__
#define _TerrainRenderer__

struct RenderPatch {
	float2 position;
	uint lod;
	float pad1;
};

struct PassData {
	uint culledPatchListIndex;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../MainEntryPoint.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p {
	float4 csPos : SV_POSITION;
	float2 uv : TEXCOORD;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	v2p output;
	output.csPos = float4(input.lsPos, 1.0f);
	output.csPos = mul(output.csPos, FrameDataCB.CurrentEditorCamera.ViewProjection);
	output.uv = input.uv;

	return output;
}

float4 PSMain(v2p input) : SV_TARGET {
	return float4(0.3f, 0.7f, 0.5f, 1.0f);
}

#endif