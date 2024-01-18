#ifndef _TerrainHeader__
#define _TerrainHeader__

struct RenderPatch {
	float2 position;
	float2 minmaxHeight;
	uint lod;
	float pad1;
	float pad2;
	float pad3;
};

struct TerrainLodDescriptor {
	uint  nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
	uint  nodeStartOffset;		// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
	uint  nodeCount;			// ��LOD�е�Node���ܸ���

	float pad1;
};

struct TerrainNodeDescriptor {
	float minHeight;	// 0 - 1
	float maxHeight;
	
	uint  tilePosX;		// 255��ʾ��Դδ����
	uint  tilePosY;		// 255��ʾ��Դδ����
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint terrainNodeIndex;						// Ŀ����νڵ�����
	TerrainNodeDescriptor terrainNodeDescriptor;	// Ŀ����νڵ�����
};

#endif