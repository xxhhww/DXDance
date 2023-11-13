#ifndef _CullStaticInstance__
#define _CullStaticInstance__

struct PassData {
	uint clusterNodeBufferIndex;				// ���е�Cluster
	uint boundingBoxBufferIndex;				// ÿһ��Instance��Ӧ��TransformedBoundingBox
	uint visibleClusterNodeIndexBufferIndex;	// �ɼ�Cluster����
	uint visibleLod0InstanceIndexBufferIndex;	// �ɼ�Instance����(Lod0)

	uint visibleLod1InstanceIndexBufferIndex;	// �ɼ�Instance����(Lod1)
	uint visibleLod2InstanceIndexBufferIndex;	// �ɼ�Instance����(Lod2)
	float pad1;
	float pad2;	
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif