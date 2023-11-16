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

// ����һ��0 - 1�����������
float rand(float3 co) {
	return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
}

#endif