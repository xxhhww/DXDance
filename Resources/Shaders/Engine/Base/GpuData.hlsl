#ifndef _IndirectData__
#define _IndirectData__

struct ItemData {
    float4x4 prevModelTrans;		    // ǰһ֡������任����
    float4x4 currModelTrans;		    // ��ǰ֡������任����
    float4   minBoundingBoxPosition;
    float4   maxBoundingBoxPosition;
};

struct ItemIndirectDrawIndexedData {
	uint2 frameDataAddress;
	uint2 passDataAddress;
	uint2 lightDataAddress;
    uint2 itemDataAddress;

    uint2 vertexBufferLocation;
    uint  vertexBufferSizeInBytes;
    uint  vertexBufferStrideInBytes;
    
    uint2 indexBufferLocation;
    uint  indexBufferSizeInBytes;
    uint  indexBufferFormat;
	
    uint  indexCountPerInstance;
    uint  instanceCount;
    uint  startIndexLocation;
    int   baseVertexLocation;

    uint  startInstanceLocation;
    float pad1;
    float pad2;
    float pad3;
};

#endif