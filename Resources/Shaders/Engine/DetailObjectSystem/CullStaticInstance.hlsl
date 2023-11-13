#ifndef _CullStaticInstance__
#define _CullStaticInstance__

struct PassData {
	uint clusterNodeBufferIndex;				// 所有的Cluster
	uint boundingBoxBufferIndex;				// 每一个Instance对应的TransformedBoundingBox
	uint visibleClusterNodeIndexBufferIndex;	// 可见Cluster索引
	uint visibleLod0InstanceIndexBufferIndex;	// 可见Instance索引(Lod0)

	uint visibleLod1InstanceIndexBufferIndex;	// 可见Instance索引(Lod1)
	uint visibleLod2InstanceIndexBufferIndex;	// 可见Instance索引(Lod2)
	float pad1;
	float pad2;	
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif