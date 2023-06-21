#ifndef _TerrainRenderer__
#define _TerrainRenderer__

#include "PublicHeader_Terrain.hlsl"

struct PassData {
	float2 worldSize;
	uint heightScale;
	uint culledPatchListIndex;
	uint heightMapIndex;
	uint normalMapIndex;
	uint lodDebug;
	float pad1;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p {
	float4 csPos : SV_POSITION;
	float3 wsPos : POSITION1;
	float3 vsPos : POSITION2;
	float2 uv : TEXCOORD;
	float2 position : POSITION3;
	uint lod : LOD;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float  viewDepth        : SV_TARGET3;
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
	heightUV *= 1.0f;
	float height = heightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r;
	input.lsPos.y = height * PassDataCB.heightScale;

	output.wsPos = input.lsPos;
	output.vsPos = mul(float4(input.lsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	output.csPos = mul(float4(input.lsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);
	output.uv = heightUV;
	output.position = patch.position;
	output.lod = lod;

	return output;
}

float3 SampleNormal(float2 uv){
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

    float3 normal;
    normal.xz = normalMap.SampleLevel(SamplerAnisotropicWrap, uv, 0).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

    return normal;
}

p2o PSMain(v2p input) {
	float3 lodDebugColor = float3(0.0f, 0.0f, 0.0f);
	if(input.lod == 0u) {
		lodDebugColor = float3(0.5f, 0.5f, 0.5f);
	}
	else if(input.lod == 1u) {
		lodDebugColor = float3(0.0f, 0.0f, 1.0f);
	}
	else if(input.lod == 2u) {
		lodDebugColor = float3(1.0f, 0.0f, 0.0f);
	}
	else if(input.lod == 3u) {
		lodDebugColor = float3(0.0f, 1.0f, 0.0f);
	}
	else if(input.lod == 4u) {
		lodDebugColor = float3(1.0f, 1.0f, 0.0f);
	}
	else if(input.lod == 5u) {
		lodDebugColor = float3(0.0f, 1.0f, 1.0f);
	}
	else { 
		lodDebugColor = float3(1.0f, 1.0f, 0.0f);
	}

	p2o output;
	output.albedoMetalness  = float4(0.5f, 0.5f, 0.5f, 0.0f);
	output.positionEmission = float4(input.wsPos, 1.0f);
	output.normalRoughness  = float4(SampleNormal(input.uv), 1.0f);
	output.viewDepth        = input.vsPos.z;

	if(PassDataCB.lodDebug) {
		output.albedoMetalness.xyz = output.albedoMetalness.xyz * 0.7f + lodDebugColor * 0.3f;
	}

	return output;
}

#endif