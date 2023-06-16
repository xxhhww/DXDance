#ifndef _PublicHeader_Terrain__
#define _PublicHeader_Terrain__

//һ��Node���8x8��Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct NodeDescriptor {
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor {
	uint nodeSize;         // ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
	uint nodeStartOffset;  // ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
	uint nodeCount;
	float pad2;
};

struct RenderPatch {
	float2 position;
	float2 minMaxHeight;
	uint lod;
	float pad1;
	float pad2;
	float pad3;
};

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

#endif