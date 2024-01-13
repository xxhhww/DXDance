#ifndef _TerrainHeader__
#define _TerrainHeader__

struct TerrainLodDescriptor {
	uint nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
	uint nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
	uint nodeCount;			// ��LOD�е�Node���ܸ���

	float pad1;
};

struct TerrainNodeDescriptor {
	uint minHeight;			// R16��ʽ
	uint maxHeight;
	
	uint tilePosX;			// 255��ʾ��Դδ����
	uint tilePosY;			// 255��ʾ��Դδ����
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint terrainNodeIndex;						// Ŀ����νڵ�����
	TerrainNodeDescriptor terrainNodeDescriptor;	// Ŀ����νڵ�����
};

#endif