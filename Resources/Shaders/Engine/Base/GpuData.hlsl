#ifndef _IndirectData__
#define _IndirectData__

struct ItemData {
    float4 prevModelTrans;		        // 前一帧的世界变换矩阵
    float4 currModelTrans;		        // 当前帧的世界变换矩阵
    float4 minBoundingBoxPosition;
    float4 maxBoundingBoxPosition;
};

struct ItemGroupPassData {
    uint  itemVertexBufferIndex;   // 顶点缓存索引
    uint  itemIndexBufferIndex;    // 索引缓存索引
    uint  itemDataBeginIndex;      // Group中第一个实例的数据在ItemDataBuffer中起始位置
    float pad1;
};

struct IndirectDrawData {
	uint2 frameDataAddress;
	uint2 passDataAddress;
	uint2 lightDataAddress;
    uint  vertexCountPerInstance;
    uint  instanceCount;
    uint  startVertexLocation;
    uint  startInstanceLocation;
}

struct IndirectDrawIndexedData {
	uint2 frameDataAddress;
	uint2 passDataAddress;
	uint2 lightDataAddress;
	uint  indexCountPerInstance;
    uint  instanceCount;
    uint  startIndexLocation;
    int   baseVertexLocation;
    uint  startInstanceLocation;
};

#endif