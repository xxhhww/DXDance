#ifndef _TerrainHeader__
#define _TerrainHeader__

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct RenderPatch {
	float2 position;
	float2 minmaxHeight;
	
	uint3  nodeLoc;				// 所属地形节点
	float  pad1;
	
	uint2  patchOffset;			// 地形节点内部偏移
	float  pad2;
	float  pad3;

	uint4  lodTrans;
};

struct TerrainLodDescriptor {
	uint  nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
	uint  nodeStartOffset;		// 该LOD中的第一个Node的开始偏移量
	uint  nodeCount;			// 该LOD中的Node的总个数

	float pad1;
};

struct TerrainNodeDescriptor {
	float minHeight;	// 0 - 1
	float maxHeight;
	
	uint  tilePosX;		// 255表示资源未加载
	uint  tilePosY;		// 255表示资源未加载
};

struct TerrainNodeGpuRuntimeState {
	uint  branch;
	float pad1;
	float pad2;
	float pad3;
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint srcTerrainNodeIndex;
	uint dstTerrainNodeIndex;						// 目标地形节点索引
	
	uint tilePosX;
	uint tilePosY;
};

struct GpuUpdateRuntimeVTAtlasRequest {
	float4x4 mvpMatrix;
	float4   tileOffset;
	float4   blendOffset;
};

struct GpuUpdateRuntimeVTPageTableRequest {
	uint  tilePosX;		// pageTable节点所需纹理对应图集中的位置
	uint  tilePosY;
	int   pageLevel;	// pageLevel
	float pad1;

	int4  rectInPage0Level;
	float4x4 mvpMatrix;	// 转换到图片空间中的矩阵
};


// ==========================================Grassland==========================================

struct GrasslandNodeDescriptor {
	uint  tileIndex;	// 255表示资源未烘焙
	float pad1;
	float pad2;
	float pad3;
};

struct GpuGrasslandNodeRequestTask {
	uint prevGrasslandNodeIndex;	// 前任地形节点索引
	uint nextGrasslandNodeIndex;	// 下任地形节点索引

	uint tileIndex;
};

#endif