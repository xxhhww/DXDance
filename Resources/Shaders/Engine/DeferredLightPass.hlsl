#ifndef _DeferredLightPass__
#define _DeferredLightPass__

struct PassData {
	uint gBufferAlbedoMetalnessMapIndex;
	uint gBufferPositionEmissionMapIndex;
	uint gBufferNormalRoughnessMapIndex;
	uint gBufferViewDepthMapIndex;
	uint finalOutputMapIndex;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<float4> gBufferAlbedoMetalnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferAlbedoMetalnessMapIndex];
	Texture2D<float4> gBufferPositionEmissionMap = ResourceDescriptorHeap[PassDataCB.gBufferPositionEmissionMapIndex];
	Texture2D<float4> gBufferNormalRoughnessMap  = ResourceDescriptorHeap[PassDataCB.gBufferNormalRoughnessMapIndex];
	Texture2D<float4> gBufferViewDepthMap        = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> finalOutputMap           = ResourceDescriptorHeap[PassDataCB.finalOutputMapIndex];

	uint2 coord = dispatchThreadID.xy;
	finalOutputMap[coord] = gBufferAlbedoMetalnessMap[coord];
}

#endif