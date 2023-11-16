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

// 返回一个0 - 1的随机浮点数
float rand(float3 co) {
	return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
}

#endif