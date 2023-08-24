#ifndef _TerrainRenderer_ForwardPlus__
#define _TerrainRenderer_ForwardPlus__

#include "TerrainHelper.hlsl"
#include "../Base/Light.hlsl"

struct PassData {
	float2 worldSize;
	uint   heightScale;
	uint   culledPatchListIndex;

	uint   heightMapIndex;
	uint   normalMapIndex;
	uint   lodDebug;
	uint   splatMapIndex;

	uint rChannelAlbedoMapIndex;
	uint rChannelNormalMapIndex;
	uint rChannelRoughnessMapIndex;
	uint rChannelDisplacementMapIndex;
			
	uint gChannelAlbedoMapIndex;
	uint gChannelNormalMapIndex;
	uint gChannelRoughnessMapIndex;
	uint gChannelDisplacementMapIndex;

	uint bChannelAlbedoMapIndex;
	uint bChannelNormalMapIndex;
	uint bChannelRoughnessMapIndex;
	uint bChannelDisplacementMapIndex;

	uint aChannelAlbedoMapIndex;
	uint aChannelNormalMapIndex;
	uint aChannelRoughnessMapIndex;
	uint aChannelDisplacementMapIndex;
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
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

float3 SampleTerrainNormalMap(float2 uv) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.normalMapIndex];

    float3 normal;
    normal.xz = normalMap.SampleLevel(SamplerAnisotropicWrap, uv, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz,normal.xz)));

    return normalize(normal);
}

float4 SampleTerrainSplatMap(float2 uv) {
	// uv.x = 1.0f - uv.x;
	uv.y = 1.0f - uv.y;

	Texture2D<float4> splatMap = ResourceDescriptorHeap[PassDataCB.splatMapIndex];

	float4 splat = splatMap.SampleLevel(SamplerLinearWrap, uv, 0u).rgba;

	return splat;
}

float3 SampleColorMapWithTriMapping(Texture2D colorMap, float3 position, float3 normal, float3 textureScale, float sharpness = 15.0f) {
		float2 uvX = position.zy * textureScale.x;
		float2 uvY = position.xz * textureScale.y;
		float2 uvZ = position.xy * textureScale.z;

		float3 weights = pow(abs(normal), sharpness);
		weights /= dot(weights, 1.0f);

		float3 xDiff = pow(colorMap.SampleLevel(SamplerLinearWrap, uvX, 0).rgb, 2.2f);
        float3 yDiff = pow(colorMap.SampleLevel(SamplerLinearWrap, uvY, 0).rgb, 2.2f);
        float3 zDiff = pow(colorMap.SampleLevel(SamplerLinearWrap, uvZ, 0).rgb, 2.2f);

		float3 color = xDiff * weights.x + yDiff * weights.y + zDiff * weights.z;

		return color;
}

float3 SampleNormalMapWithTriMapping(Texture2D normalMap, float3 position, float3 normal, float3 textureScale, float sharpness = 15.0f) {
		float2 uvX = position.zy * textureScale.x;
		float2 uvY = position.xz * textureScale.y;
		float2 uvZ = position.xy * textureScale.z;

		float3 weights = pow(abs(normal), sharpness);
		weights /= dot(weights, 1.0f);

		float3 tnormalX = normalMap.SampleLevel(SamplerLinearWrap, uvX, 0).xyz * 2.0f - 1.0f;
        float3 tnormalY = normalMap.SampleLevel(SamplerLinearWrap, uvY, 0).xyz * 2.0f - 1.0f;
        float3 tnormalZ = normalMap.SampleLevel(SamplerLinearWrap, uvZ, 0).xyz * 2.0f - 1.0f;

		tnormalX = float3(
			tnormalX.xy + normal.zy,
			abs(tnormalX.z) * normal.x
			);
		tnormalY = float3(
			tnormalY.xy + normal.xz,
			abs(tnormalY.z) * normal.y
			);
		tnormalZ = float3(
			tnormalZ.xy + normal.xy,
			abs(tnormalZ.z) * normal.z
			);

		float3 wsNormal = normalize(
			tnormalX.zyx * weights.x +
			tnormalY.xzy * weights.y +
			tnormalZ.xyz * weights.z
		);

		return wsNormal;
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

	float3 wsPos = input.wsPos;
	float3 wsNormal = SampleTerrainNormalMap(input.uv);
	float4 splat = SampleTerrainSplatMap(input.uv);

	float4 albedo = float4(0.5f, 0.5f, 0.5f, 1.0f);
	float  roughness = 0.99f;

	Texture2D<float4> rChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.rChannelAlbedoMapIndex];
	Texture2D<float4> rChannelNormalMap = ResourceDescriptorHeap[PassDataCB.rChannelNormalMapIndex];

	Texture2D<float4> gChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.gChannelAlbedoMapIndex];
	Texture2D<float4> gChannelNormalMap = ResourceDescriptorHeap[PassDataCB.gChannelNormalMapIndex];

	Texture2D<float4> bChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.bChannelAlbedoMapIndex];
	Texture2D<float4> bChannelNormalMap = ResourceDescriptorHeap[PassDataCB.bChannelNormalMapIndex];

	Texture2D<float4> aChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.aChannelAlbedoMapIndex];
	Texture2D<float4> aChannelNormalMap = ResourceDescriptorHeap[PassDataCB.aChannelNormalMapIndex];

	float  textureScale = 1.0f / 64.0f;

	float3 rChannel = SampleColorMapWithTriMapping(rChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 gChannel = SampleColorMapWithTriMapping(gChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 bChannel = SampleColorMapWithTriMapping(bChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 aChannel = SampleColorMapWithTriMapping(aChannelAlbedoMap, wsPos, wsNormal, textureScale);
	albedo.rgb = splat.r * rChannel + splat.g * gChannel + splat.b * bChannel + splat.a * aChannel;

	rChannel = SampleNormalMapWithTriMapping(rChannelNormalMap, wsPos, wsNormal, textureScale);
	gChannel = SampleNormalMapWithTriMapping(gChannelNormalMap, wsPos, wsNormal, textureScale);
	bChannel = SampleNormalMapWithTriMapping(bChannelNormalMap, wsPos, wsNormal, textureScale);
	aChannel = SampleNormalMapWithTriMapping(aChannelNormalMap, wsPos, wsNormal, textureScale);
	wsNormal.xyz = normalize(splat.r * rChannel + splat.g * gChannel + splat.b * bChannel + splat.a * aChannel);

	/*
	float  groundGrassMapScale = 0.05f;
	float  groundRockMapScale  = 0.01f;

	TriplanarMapping tri;
	tri.initialize(input.wsPos, wsNormal, float3(groundRockMapScale, groundGrassMapScale, groundRockMapScale), 15.0f);

	float2 groundUV = tri.uvY;

	Texture2D<float4> splatMap                = ResourceDescriptorHeap[PassDataCB.splatMapIndex];

	Texture2D<float4> groundGrassAlbedoMap    = ResourceDescriptorHeap[PassDataCB.rChannelAlbedoMapIndex];
	Texture2D<float4> groundGrassNormalMap    = ResourceDescriptorHeap[PassDataCB.rChannelNormalMapIndex];

	Texture2D<float4> groundRockAlbedoMap     = ResourceDescriptorHeap[PassDataCB.gChannelAlbedoMapIndex];
	Texture2D<float4> groundRockNormalMap     = ResourceDescriptorHeap[PassDataCB.gChannelNormalMapIndex];

	float4 albedo =
		pow(groundRockAlbedoMap.SampleLevel(SamplerLinearWrap, tri.uvX, 0.0f), 2.2f) * tri.weights.x +
		pow(groundGrassAlbedoMap.SampleLevel(SamplerLinearWrap, groundUV, 0.0f), 2.2f) * tri.weights.y +
		pow(groundRockAlbedoMap.SampleLevel(SamplerLinearWrap, tri.uvZ, 0.0f), 2.2f) * tri.weights.z;

	float roughness =
		groundRockRoughnessMap.SampleLevel(SamplerLinearWrap, tri.uvX, 0.0f) * tri.weights.x +
		groundGrassRoughnessMap.SampleLevel(SamplerLinearWrap, groundUV, 0.0f) * tri.weights.y +
		groundRockRoughnessMap.SampleLevel(SamplerLinearWrap, tri.uvZ, 0.0f) * tri.weights.z;
	roughness = 0.99f;

	float3 tnormalX = SampleNormalMap(groundRockNormalMap, SamplerLinearWrap, tri.uvX);
	float3 tnormalY = SampleNormalMap(groundGrassNormalMap, SamplerLinearWrap, groundUV);
	float3 tnormalZ = SampleNormalMap(groundRockNormalMap, SamplerLinearWrap, tri.uvZ);

	float3 finalWsNormal = tri.normalmap(wsNormal, tnormalX, tnormalY, tnormalZ);
	*/

	Surface surface;

	surface.albedo = albedo;
	surface.normal = wsNormal;
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
	p2o output;
	output.shadingResult   = totalLighting.evaluate(surface.albedo);
	output.normalRoughness = float4(wsNormal, roughness);
	output.screenVelocity  = float2(velocity.xy);

	return output;
}

#endif