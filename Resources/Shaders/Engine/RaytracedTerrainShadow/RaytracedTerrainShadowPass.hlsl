#ifndef _RaytracedTerrainShadowPass__
#define _RaytracedTerrainShadowPass__

struct PassData {
	uint  terrainHeightMapIndex;
	uint  gBufferPositionEmissionMapIndex;
	uint  ssRaytracedTerrainShadowMapIndex;
	float pad1;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	uint2 pixelIndex = dispatchThreadID.xy;

	Texture2D<float4>   terrainHeightMap            = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];
	Texture2D<float4>   gBufferPositionEmissionMap  = ResourceDescriptorHeap[PassDataCB.gBufferPositionEmissionMapIndex];
	RWTexture2D<float>  ssRaytracedTerrainShadowMap = ResourceDescriptorHeap[PassDataCB.ssRaytracedTerrainShadowMapIndex];

	float3 wsPosition = gBufferPositionEmissionMap[pixelIndex].xyz;
	float3 sunDirection = LightDataSB[0].position.xyz;
}

#endif