#ifndef _CascadeShadowRedirectPass__
#define _CascadeShadowRedirectPass__

struct PassData {
	uint  redirectedIndirectArgsIndex;	// 需要重定向的IndirectArgs索引
	uint2 targetAddress;				// 目标地址
	float pad1;	
}

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	RWStructuredBuffer<ItemIndirectDrawIndexedData> cascadeShadowIndirectArgs = ResourceDescriptorHeap[PassDataCB.redirectedIndirectArgsIndex];
	cascadeShadowIndirectArgs[dispatchThreadID.x].passDataAddress = PassDataCB.targetAddress;
}

#endif