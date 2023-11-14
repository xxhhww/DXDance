#ifndef _StaticInstanceHelper__
#define _StaticInstanceHelper__

struct ClusterNode {
	float4 minBoundingBoxPosition;
	float4 maxBoundingBoxPosition;

	int firstChildIndex;	// 首孩子
	int lastChildIndex;		// 尾孩子

	int firstInstanceIndex;	// 首instance
	int lastInstanceIndex;	// 尾instance

	float4 minInstanceScale;	// 最小缩放比例
	float4 maxInstanceScale;	// 最大缩放比例
};

#endif