#ifndef _DeferredLightPass__
#define _DeferredLightPass__

struct PassData {
	float4 halton;
	uint rngSeedMapIndex;
	uint blueNoise3DMapIndex;
	uint gBufferAlbedoMetalnessMapIndex;
	uint gBufferPositionEmissionMapIndex;
	uint gBufferNormalRoughnessMapIndex;
	uint gBufferViewDepthMapIndex;
	uint finalOutputMapIndex;
	float pad1;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"
#include "Base/ShadingCommon.hlsl"
#include "Math/Matrix.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<uint4>  rngSeedMap                 = ResourceDescriptorHeap[PassDataCB.rngSeedMapIndex];
	Texture3D<float4> blueNoise3DMap             = ResourceDescriptorHeap[PassDataCB.blueNoise3DMapIndex];
	Texture2D<float4> gBufferAlbedoMetalnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferAlbedoMetalnessMapIndex];
	Texture2D<float4> gBufferPositionEmissionMap = ResourceDescriptorHeap[PassDataCB.gBufferPositionEmissionMapIndex];
	Texture2D<float4> gBufferNormalRoughnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferNormalRoughnessMapIndex];
	Texture2D<float4> gBufferViewDepthMap        = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> finalOutputMap           = ResourceDescriptorHeap[PassDataCB.finalOutputMapIndex];

	uint2 coord = dispatchThreadID.xy;
	
	GBufferSurface gBufferSurface;
	gBufferSurface.albedo    = gBufferAlbedoMetalnessMap[coord].xyz;
	gBufferSurface.position  = gBufferPositionEmissionMap[coord].xyz;
	gBufferSurface.normal    = gBufferNormalRoughnessMap[coord].xyz;
	gBufferSurface.roughness = gBufferNormalRoughnessMap[coord].w;
	gBufferSurface.metalness = gBufferAlbedoMetalnessMap[coord].w;

	bool emission = gBufferPositionEmissionMap[coord].w;

	RandomSequences randomSequences;
    randomSequences.blueNoise = blueNoise3DMap[rngSeedMap[coord].xyz];
    randomSequences.halton    = PassDataCB.halton;
	
	float3 viewDirection = normalize(FrameDataCB.CurrentEditorCamera.Position.xyz - gBufferSurface.position.xyz);

	ShadingResult shadingResult = ZeroShadingResult();

	for(uint i = 0u; i < FrameDataCB.lightSize; i++) {
		Light light = LightDataSB[i];

		if(light.type == 0) {
			// Sun Light
			ShadeWithSunLight(gBufferSurface, light, randomSequences, viewDirection, shadingResult);
		}
	}


	finalOutputMap[coord] = float4(shadingResult.analyticUnshadowedOutgoingLuminance, 1.0f);
}

#endif