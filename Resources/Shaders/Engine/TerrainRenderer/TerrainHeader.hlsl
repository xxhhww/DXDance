#ifndef _TerrainHeader__
#define _TerrainHeader__

struct TerrainLodDescriptor {
	uint nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
	uint nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
	uint nodeCount;			// 该LOD中的Node的总个数

	float pad1;
};

struct TerrainNodeDescriptor {
	uint minHeight;			// R16格式
	uint maxHeight;
	
	uint tilePosX;			// 255表示资源未加载
	uint tilePosY;			// 255表示资源未加载
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint terrainNodeIndex;						// 目标地形节点索引
	TerrainNodeDescriptor terrainNodeDescriptor;	// 目标地形节点描述
};

#endif