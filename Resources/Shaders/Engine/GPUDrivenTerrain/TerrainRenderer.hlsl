#ifndef _TerrainRenderer__
#define _TerrainRenderer__

#include "TerrainHelper.hlsl"

struct PassData {
	float2 worldSize;
	uint heightScale;
	uint culledPatchListIndex;

	uint heightMapIndex;
	uint normalMapIndex;
	uint lodDebug;
	float pad1;

	uint groundGrassAlbedoMapIndex;
	uint groundGrassNormalMapIndex;
	uint groundGrassRoughnessMapIndex;
	uint groundGrassDisplacementMapIndex;

	uint groundRockAlbedoMapIndex;
	uint groundRockNormalMapIndex;
	uint groundRockRoughnessMapIndex;
	uint groundRockDisplacementMapIndex;

	uint groundMudAlbedoMapIndex;
	uint groundMudNormalMapIndex;
	uint groundMudRoughnessMapIndex;
	uint groundMudDisplacementMapIndex;
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
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
	float2 uv        : TEXCOORD;
	float2 position  : POSITION4;
	uint   lod       : LOD;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3;
	float  viewDepth        : SV_TARGET4;
};

float3 SampleTerrainNormalMap(float2 uv) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

    float3 normal;
    normal.xz = normalMap.SampleLevel(SamplerAnisotropicWrap, uv, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

    return normalize(normal);
}

float3 SampleNormalMap(Texture2D<float4> normalMap, SamplerState s, float2 uv) {
    float3 N = normalMap.SampleLevel(s, uv, 0.0f).xyz;
    bool reconstructZ = N.z == 0.f;
    N = N * 2.f - 1.f;

    if (reconstructZ) {
        N.z = sqrt(1.f - dot(N.xy, N.xy));
    }

    return N;
}

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
	output.uv = heightUV;
	output.position = patch.position;
	output.lod = lod;

	return output;
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

	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);

    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);

    float3 velocity = currUVSpacePos - prevUVSpacePos;

	float  groundGrassMapScale = 1.0f;
	float  groundRockMapScale  = 0.005f;

	float3 wsNormal = SampleTerrainNormalMap(input.uv);

	TriplanarMapping tri;
	tri.initialize(input.wsPos, wsNormal, float3(groundRockMapScale, groundGrassMapScale, groundRockMapScale), 15.0f);

	float2 groundUV = tri.uvY;

	Texture2D<float4> groundGrassAlbedoMap    = ResourceDescriptorHeap[PassDataCB.groundGrassAlbedoMapIndex];
	Texture2D<float4> groundGrassNormalMap    = ResourceDescriptorHeap[PassDataCB.groundGrassNormalMapIndex];
	Texture2D<float>  groundGrassRoughnessMap = ResourceDescriptorHeap[PassDataCB.groundGrassRoughnessMapIndex];

	Texture2D<float4> groundRockAlbedoMap     = ResourceDescriptorHeap[PassDataCB.groundRockAlbedoMapIndex];
	Texture2D<float4> groundRockNormalMap     = ResourceDescriptorHeap[PassDataCB.groundRockNormalMapIndex];
	Texture2D<float>  groundRockRoughnessMap  = ResourceDescriptorHeap[PassDataCB.groundRockRoughnessMapIndex];

	float4 albedo =
		groundRockAlbedoMap.SampleLevel(SamplerLinearWrap, tri.uvX, 0.0f) * tri.weights.x +
		groundGrassAlbedoMap.SampleLevel(SamplerLinearWrap, groundUV, 0.0f) * tri.weights.y +
		groundRockAlbedoMap.SampleLevel(SamplerLinearWrap, tri.uvZ, 0.0f) * tri.weights.z;

	float roughness =
		groundRockRoughnessMap.SampleLevel(SamplerLinearWrap, tri.uvX, 0.0f) * tri.weights.x +
		groundGrassRoughnessMap.SampleLevel(SamplerLinearWrap, groundUV, 0.0f) * tri.weights.y +
		groundRockRoughnessMap.SampleLevel(SamplerLinearWrap, tri.uvZ, 0.0f) * tri.weights.z;

	float3 tnormalX = SampleNormalMap(groundRockNormalMap, SamplerLinearWrap, tri.uvX);
	float3 tnormalY = SampleNormalMap(groundGrassNormalMap, SamplerLinearWrap, groundUV);
	float3 tnormalZ = SampleNormalMap(groundRockNormalMap, SamplerLinearWrap, tri.uvZ);

	wsNormal = tri.normalmap(wsNormal, tnormalX, tnormalY, tnormalZ);

	p2o output;
	output.albedoMetalness  = float4(albedo.rgb, 0.0f);
	output.positionEmission = float4(input.wsPos, 0.0f);
	output.normalRoughness  = float4(wsNormal, 0.99f);
	output.motionVector     = float4(velocity, 0.0f);
	output.viewDepth        = input.vsPos.z;

	if(PassDataCB.lodDebug) {
		output.albedoMetalness.xyz = output.albedoMetalness.xyz * 0.7f + lodDebugColor * 0.3f;
	}

	return output;
}

#endif