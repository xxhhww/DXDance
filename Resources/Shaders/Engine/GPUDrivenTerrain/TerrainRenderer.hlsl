#ifndef _TerrainRenderer__
#define _TerrainRenderer__

struct RenderPatch {
	float2 position;
	float2 minMaxHeight;
	uint lod;
	float pad1;
	float pad2;
	float pad3;
};

struct PassData {
	float2 worldSize;
	uint heightScale;
	uint culledPatchListIndex;
	uint heightMapIndex;
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
	float2 position : PATCHPOS;
	uint lod : LOD;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch> culledPatchList = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];
	Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.heightMapIndex];

	v2p output;

	RenderPatch patch = culledPatchList[instanceID];
	uint lod = patch.lod;
	float scale = pow(2,lod);
	input.lsPos.xz *= scale;
	input.lsPos.xz += patch.position;

	float2 heightUV = (input.lsPos.xz + (PassDataCB.worldSize * 0.5f) + 0.5f) / (PassDataCB.worldSize + 1.0f);
	float height = heightMap.SampleLevel(SamplerLinearClamp, heightUV, 0u).r;
	input.lsPos.y = height * PassDataCB.heightScale;

	output.csPos = float4(input.lsPos, 1.0f);
	output.csPos = mul(output.csPos, FrameDataCB.CurrentEditorCamera.ViewProjection);
	output.uv = input.uv;
	output.position = patch.position;
	output.lod = lod;

	return output;
}

float4 PSMain(v2p input) : SV_TARGET {
	/*
	if(input.lod == 0 || input.lod == 2 || input.lod == 4) {
		if(input.position.x > 0 && input.position.y > 0){
			return float4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else if(input.position.x > 0 && input.position.y < 0){
			return float4(0.0f, 1.0f, 0.0f, 1.0f);
		}
		else if(input.position.x < 0 && input.position.y > 0){
			return float4(0.0f, 0.0f, 1.0f, 1.0f);
		}
		else if(input.position.x < 0 && input.position.y < 0){
			return float4(0.5f, 0.5f, 0.5f, 1.0f);
		}
		return float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(input.lod == 1 || input.lod == 3 || input.lod == 5) {
		if(input.position.x < 0 && input.position.y < 0){
			return float4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else if(input.position.x < 0 && input.position.y > 0){
			return float4(0.0f, 1.0f, 0.0f, 1.0f);
		}
		else if(input.position.x > 0 && input.position.y < 0){
			return float4(0.0f, 0.0f, 1.0f, 1.0f);
		}
		else if(input.position.x > 0 && input.position.y > 0){
			return float4(0.5f, 0.5f, 0.5f, 1.0f);
		}
		return float4(1.0f, 0.0f, 1.0f, 1.0f);
	}
	*/
	if(input.lod == 0u){
		return float4(0.5f, 0.5f, 0.5f, 1.0f);
	}
	else if(input.lod == 1u){
		return float4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	else if(input.lod == 2u){
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if(input.lod == 3u){
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if(input.lod == 4u){
		return float4(1.0f, 1.0f, 0.0f, 1.0f);
	}
	else if(input.lod == 5u){
		return float4(0.0f, 1.0f, 1.0f, 1.0f);
	}
	return float4(1.0f, 1.0f, 0.0f, 1.0f);
	
}

#endif