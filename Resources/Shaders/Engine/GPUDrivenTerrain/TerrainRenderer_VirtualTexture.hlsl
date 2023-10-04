#ifndef _TerrainRenderer_VirtualTexture__
#define _TerrainRenderer_VirtualTexture__

#include "TerrainHelper.hlsl"
#include "../Base/Light.hlsl"

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
	uint splatMapIndex;

	uint pageTableMapIndex;
	uint physicalAlbedoMapIndex;
	uint physicalNormalMapIndex;
	float pad1;

	uint rChannelAlbedoMapIndex;
	uint rChannelNormalMapIndex;
	uint rChannelRoughnessMapIndex;
	uint rChannelHeightMapIndex;
			
	uint gChannelAlbedoMapIndex;
	uint gChannelNormalMapIndex;
	uint gChannelRoughnessMapIndex;
	uint gChannelHeightMapIndex;

	uint bChannelAlbedoMapIndex;
	uint bChannelNormalMapIndex;
	uint bChannelRoughnessMapIndex;
	uint bChannelHeightMapIndex;

	uint aChannelAlbedoMapIndex;
	uint aChannelNormalMapIndex;
	uint aChannelRoughnessMapIndex;
	uint aChannelHeightMapIndex;
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
	float2 uvHeight  : TEXCOORD0;
	float2 uvVT      : TEXCOORD1;
	float2 uv        : TEXCOORD2;
	uint   lod       : LOD;
};

struct p2o {
	float4 shadingResult	: SV_TARGET0;
	float4 normalRoughness	: SV_TARGET1;
	float2 screenVelocity	: SV_TARGET2;
	uint4  terrainFeedback	: SV_TARGET3;
};

float3 SampleTerrainNormalMap(float2 uv) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

    float3 normal;
    normal.xz = normalMap.SampleLevel(SamplerAnisotropicWrap, uv, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

    return normalize(normal);
}

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
	heightUV.y = 1.0f - heightUV.y;
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
	float3 lodDebugColor = GetLODColor(input.lod);

	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);

    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);

    float3 velocity = currUVSpacePos - prevUVSpacePos;

	Texture2D pageTableMap = ResourceDescriptorHeap[PassDataCB.pageTableMapIndex];
	Texture2D physicalAlbedoMap = ResourceDescriptorHeap[PassDataCB.physicalAlbedoMapIndex];
	Texture2D physicalNormalMap = ResourceDescriptorHeap[PassDataCB.physicalNormalMapIndex];

	// Sample Terrain Normal
	float3 wsNormal = SampleTerrainNormalMap(input.uvHeight);

	// 方便调试
	float  paddingSize = PassDataCB.vtPhysicalMapParams.x;
	float  tileSize = PassDataCB.vtPhysicalMapParams.y;
	float2 physicalMapSize = PassDataCB.vtPhysicalMapParams.z;
	float  tileSizeWithPadding = tileSize + paddingSize * 2.0f;
	float  tileCountPerAxisInPhysicalMap = physicalMapSize.x / tileSizeWithPadding;

	// mip 0下一个tile对应的世界空间中的大小
	float  tileSizeMip0InWorldSpace = PassDataCB.worldSize.x / PassDataCB.vtFeedbackParams.x;

	float2 uvVT = (input.wsPos.xz - PassDataCB.vtRealRect.xy) / PassDataCB.vtRealRect.zw;

	// Look up index info in pageTableMap
	// uvVT以左下角为原点，而PageTableMap与PhysicalMap都以左上角为原点
	float2 pageTableUV = float2(uvVT.x, 1.0f - uvVT.y);
	uint2 pageTableIndex = floor(pageTableUV * PassDataCB.vtFeedbackParams.x);
	pageTableIndex.x = clamp(pageTableIndex.x, 0, PassDataCB.vtFeedbackParams.x - 1);
	pageTableIndex.y = clamp(pageTableIndex.y, 0, PassDataCB.vtFeedbackParams.x - 1);
	float4 indexInfo = pageTableMap.mips[0][pageTableIndex.xy].rgba;

	// 目标区域的纹理未渲染到Tile上
	float4 albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
	float3 normal = float3(0.0f, 1.0f, 0.0f);
	float roughness = 0.99f;

	if(indexInfo.w == 0.0f) {
		albedo.rgb = float3(0.0f, 0.0f, 0.0f);
		normal = wsNormal;
	}
	else if(indexInfo.w == 1.0f) {
		float2 pageOrigin = float2(indexInfo.x, tileCountPerAxisInPhysicalMap - indexInfo.y) * tileSizeWithPadding;
		float  tileSizeMipNInWorldSpace = pow(2, indexInfo.z) * tileSizeMip0InWorldSpace;
		float2 scaleTileSize = (input.wsPos.xz + (PassDataCB.worldSize / 2.0f)) / tileSizeMipNInWorldSpace;
		// 当前像素点对应的uvVT在Page中的偏移
		float2 pixelInPageOffset = frac(scaleTileSize) * tileSize + paddingSize;

		float2 uvPT = float2(pageOrigin.x + pixelInPageOffset.x, pageOrigin.y - pixelInPageOffset.y) / physicalMapSize;

		albedo.rgb = physicalAlbedoMap.SampleLevel(SamplerLinearWrap, uvPT, 0u).rgb;
		// normal = physicalNormalMap.SampleLevel(SamplerLinearWrap, uvPT, 0u).rgb;
	}
	normal = wsNormal;

	Surface surface;
	surface.albedo = albedo;
	surface.normal = normal;
	surface.roughness = roughness;
	surface.metallic = 0.0f;
	surface.emission = 0.0f;

	surface.position = input.wsPos;
	float3 camToP = surface.position - FrameDataCB.CurrentEditorCamera.Position.xyz;
	surface.viewDir = -normalize(camToP);

	surface.InferRemainingProperties();

	LightContribution totalLighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	Light sunLight = LightDataSB[0];

	totalLighting.addSunLight(surface, sunLight/*, screenUV, pixelDepth,
		shadowMap, shadowSampler, lighting.shadowMapTexelSize, sssTexture, clampSampler*/);

	// Calcute Feedback
	uint2 page = floor(input.uvVT * PassDataCB.vtFeedbackParams.x);
	
	float2 uv = input.uvVT * PassDataCB.vtFeedbackParams.y;
	float2 dx = ddx(uv);
	float2 dy = ddy(uv);
	int mip = clamp(int(0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + PassDataCB.vtFeedbackParams.w), 0, PassDataCB.vtFeedbackParams.z);

	// 是否越界
	uint overBound = (input.uvVT.x > 1.0f || input.uvVT.y > 1.0f) ? 0u : 1u;

	p2o output;
	output.shadingResult   = totalLighting.evaluate(surface.albedo);
	// output.shadingResult = float4(lodDebugColor, 1.0f);
	// output.shadingResult   = float4(indexInfo.z * 0.1f, 0.0f, 0.0f, 1.0f);
	output.normalRoughness = float4(normal, roughness);
	output.screenVelocity  = float2(velocity.xy);
	output.terrainFeedback = uint4(page, mip , overBound);

	return output;
}

#endif