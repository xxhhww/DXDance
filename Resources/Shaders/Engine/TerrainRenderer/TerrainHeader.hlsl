#ifndef _TerrainHeader__
#define _TerrainHeader__

//һ��Node���8x8��Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct RenderPatch {
	float2 position;
	float2 minmaxHeight;
	
	uint3  nodeLoc;				// �������νڵ�
	float  pad1;
	
	uint2  patchOffset;			// ���νڵ��ڲ�ƫ��
	float  pad2;
	float  pad3;

	uint4  lodTrans;
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

struct TerrainNodeGpuRuntimeState {
	uint  branch;
	float pad1;
	float pad2;
	float pad3;
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint srcTerrainNodeIndex;
	uint dstTerrainNodeIndex;						// Ŀ����νڵ�����
	
	uint tilePosX;
	uint tilePosY;
};

struct GpuUpdateRuntimeVTAtlasRequest {
	float4x4 mvpMatrix;
	float4   tileOffset;
	float4   blendOffset;
};

struct GpuUpdateRuntimeVTPageTableRequest {
	uint  tilePosX;		// pageTable�ڵ����������Ӧͼ���е�λ��
	uint  tilePosY;
	int   pageLevel;	// pageLevel
	float pad1;

	int4  rectInPage0Level;
	float4x4 mvpMatrix;	// ת����ͼƬ�ռ��еľ���
};


// ==========================================Grassland==========================================

struct GrasslandNodeDescriptor {
	uint  tileIndex;	// 255��ʾ��Դδ�決
	float pad1;
	float pad2;
	float pad3;
};

struct GpuGrasslandNodeRequestTask {
	uint prevGrasslandNodeIndex;	// ǰ�ε��νڵ�����
	uint nextGrasslandNodeIndex;	// ���ε��νڵ�����

	uint tileIndex;
};

#endif