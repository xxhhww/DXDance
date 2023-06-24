#ifndef _DeferredLightPass__
#define _DeferredLightPass__

struct PassData {
	float4 halton;
	uint rngSeedMapIndex;
	uint blueNoise3DMapIndex;
	uint skyLuminanceMapIndex;
	uint gBufferAlbedoMetalnessMapIndex;
	uint gBufferPositionEmissionMapIndex;
	uint gBufferNormalRoughnessMapIndex;
	uint gBufferViewDepthMapIndex;
	uint finalOutputMapIndex;
	uint2 finalOutputMapSize;
	float pad1;
	float pad2;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"
#include "Base/ShadingCommon.hlsl"
#include "Base/Utils.hlsl"
#include "Math/Matrix.hlsl"

ShadingResult GetSkyShadingResult(float2 pixelUV) {
	Texture2D<float4> skyLuminanceMap = ResourceDescriptorHeap[PassDataCB.skyLuminanceMapIndex];

    float3 pointInfronOfCamera = NDCDepthToWorldPosition(1.0, pixelUV, FrameDataCB.CurrentEditorCamera);
    float3 worldViewDirection = normalize(pointInfronOfCamera - FrameDataCB.CurrentEditorCamera.Position.xyz);
    float2 skyUV = (OctEncode(worldViewDirection) + 1.0) * 0.5;
    float3 luminance = skyLuminanceMap.SampleLevel(SamplerLinearClamp, skyUV, 0).rgb;

    ShadingResult result = ZeroShadingResult();
    result.analyticUnshadowedOutgoingLuminance = luminance;
    return result;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<uint4>  rngSeedMap                 = ResourceDescriptorHeap[PassDataCB.rngSeedMapIndex];
	Texture3D<float4> blueNoise3DMap             = ResourceDescriptorHeap[PassDataCB.blueNoise3DMapIndex];
	Texture2D<float4> gBufferAlbedoMetalnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferAlbedoMetalnessMapIndex];
	Texture2D<float4> gBufferPositionEmissionMap = ResourceDescriptorHeap[PassDataCB.gBufferPositionEmissionMapIndex];
	Texture2D<float4> gBufferNormalRoughnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferNormalRoughnessMapIndex];
	Texture2D<float4> gBufferViewDepthMap        = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> finalOutputMap           = ResourceDescriptorHeap[PassDataCB.finalOutputMapIndex];
	Texture2D<float4> skyLuminanceMap = ResourceDescriptorHeap[PassDataCB.skyLuminanceMapIndex];


	uint2 pixelIndex = dispatchThreadID.xy;
	float2 pixelUV = TexelIndexToUV(pixelIndex, PassDataCB.finalOutputMapSize);
	
	GBufferSurface gBufferSurface;
	gBufferSurface.albedo    = gBufferAlbedoMetalnessMap[pixelIndex].xyz;
	gBufferSurface.position  = gBufferPositionEmissionMap[pixelIndex].xyz;
	gBufferSurface.normal    = gBufferNormalRoughnessMap[pixelIndex].xyz;
	gBufferSurface.roughness = gBufferNormalRoughnessMap[pixelIndex].w;
	gBufferSurface.metalness = gBufferAlbedoMetalnessMap[pixelIndex].w;
	float viewDepth = gBufferViewDepthMap[pixelIndex].x;

	ShadingResult shadingResult = ZeroShadingResult();

	// Sky Detection
	if (viewDepth >= FrameDataCB.CurrentEditorCamera.FarPlane) {
        shadingResult = GetSkyShadingResult(pixelUV);
		finalOutputMap[pixelIndex] = float4(shadingResult.analyticUnshadowedOutgoingLuminance, 1.0f);
        return;
    }

	bool emission = gBufferPositionEmissionMap[pixelIndex].w;

	RandomSequences randomSequences;
    randomSequences.blueNoise = blueNoise3DMap[rngSeedMap[pixelIndex].xyz];
    randomSequences.halton    = PassDataCB.halton;
	
	float3 viewDirection = normalize(FrameDataCB.CurrentEditorCamera.Position.xyz - gBufferSurface.position.xyz);

	for(uint i = 0u; i < FrameDataCB.lightSize; i++) {
		Light light = LightDataSB[i];

		if(light.type == 0) {
			// Sun Light
			ShadeWithSunLight(gBufferSurface, light, randomSequences, viewDirection, shadingResult);
		}
	}

	finalOutputMap[pixelIndex] = float4(shadingResult.analyticUnshadowedOutgoingLuminance, 1.0f);
}

#endif