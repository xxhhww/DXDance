#ifndef _GpuCullingHelper__
#define _GpuCullingHelper__

struct ItemData {
	float4x4 prevModelTrans;	// ǰһ֡������任����
	float4x4 currModelTrans;    // ��ǰ֡������任����
    float4 center;              // boundingBox
    float4 extend;
};

struct IndirectCommand {
    uint2 frameDataAddress;
    uint2 lightDataAddress;
    uint2 perItemDataAddress;
    uint4 vertexBufferView;
    uint4 indexBufferView;
    // ���Ʋ���
    uint4 drawArguments1;
    uint drawArguments2;
};

#endif