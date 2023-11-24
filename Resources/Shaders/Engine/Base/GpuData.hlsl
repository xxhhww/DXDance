#ifndef _IndirectData__
#define _IndirectData__

struct ItemData {
    float4 prevModelTrans;		        // ǰһ֡������任����
    float4 currModelTrans;		        // ��ǰ֡������任����
    float4 minBoundingBoxPosition;
    float4 maxBoundingBoxPosition;
};

struct ItemGroupPassData {
    uint  itemVertexBufferIndex;   // ���㻺������
    uint  itemIndexBufferIndex;    // ������������
    uint  itemDataBeginIndex;      // Group�е�һ��ʵ����������ItemDataBuffer����ʼλ��
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