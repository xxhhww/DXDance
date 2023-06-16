#ifndef _PublicHeader_Terrain__
#define _PublicHeader_Terrain__

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct NodeDescriptor {
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor {
	uint nodeSize;         // 该LOD中每一个Node的边长(米)(Node是正方形)
	uint nodeStartOffset;  // 该LOD中的第一个Node的开始偏移量
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