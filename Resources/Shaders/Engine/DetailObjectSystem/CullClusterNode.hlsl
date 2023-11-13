#ifndef _CullClusterNode__
#define _CullClusterNode__

struct PassData {
	uint clusterNodeBufferIndex;				// 所有的Cluster
	uint visibleClusterNodeIndexBufferIndex;	// 可见Cluster索引
	float pad1;
	float pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif