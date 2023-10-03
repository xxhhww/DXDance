#ifndef _GpuCullingHelper__
#define _GpuCullingHelper__

struct ItemData {
	float4x4 prevModelTrans;	// 前一帧的世界变换矩阵
	float4x4 currModelTrans;    // 当前帧的世界变换矩阵
    float4 center;              // boundingBox
    float4 extend;
};

struct IndirectCommand {
    uint2 frameDataAddress;
    uint2 lightDataAddress;
    uint2 perItemDataAddress;
    uint4 vertexBufferView;
    uint4 indexBufferView;
    // 绘制参数
    uint4 drawArguments1;
    uint drawArguments2;
};

#endif