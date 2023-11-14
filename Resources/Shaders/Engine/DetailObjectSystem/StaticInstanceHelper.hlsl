#ifndef _StaticInstanceHelper__
#define _StaticInstanceHelper__

struct ClusterNode {
	float4 minBoundingBoxPosition;
	float4 maxBoundingBoxPosition;

	int firstChildIndex;	// �׺���
	int lastChildIndex;		// β����

	int firstInstanceIndex;	// ��instance
	int lastInstanceIndex;	// βinstance

	float4 minInstanceScale;	// ��С���ű���
	float4 maxInstanceScale;	// ������ű���
};

#endif