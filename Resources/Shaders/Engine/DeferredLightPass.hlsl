#ifndef _DeferredLightPass__
#define _DeferredLightPass__

struct PassData {
	uint _GBufferAlbedoMapIndex;
	uint _GBufferPositionMapIndex;
	uint _GBufferNormalMapIndex;
	uint _GBufferMREMapIndex;
	uint _FinalOutputMapIndex;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<float4> _GBufferAlbedoMap   = ResourceDescriptorHeap[PassDataCB._GBufferAlbedoMapIndex];
	Texture2D<float4> _GBufferPositionMap = ResourceDescriptorHeap[PassDataCB._GBufferPositionMapIndex];
	Texture2D<float4> _GBufferNormalMap   = ResourceDescriptorHeap[PassDataCB._GBufferNormalMapIndex];
	Texture2D<float4> _GBufferMREMap      = ResourceDescriptorHeap[PassDataCB._GBufferMREMapIndex];
	RWTexture2D<float4> _FinalOutputMap   = ResourceDescriptorHeap[PassDataCB._FinalOutputMapIndex];

	uint2 coord = dispatchThreadID.xy;
	_FinalOutputMap[coord] = _GBufferAlbedoMap[coord];
}

#endif