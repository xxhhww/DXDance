#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/BoundingBox.h"

#include <array>
#include <vector>

namespace Renderer {

    struct GpuCameraData {
    public:
        Math::Vector4 position;
        Math::Vector4 lookUp;
        // 16 byte boundary
        Math::Matrix4 view;
        Math::Matrix4 projection;
        Math::Matrix4 viewProjection;
        Math::Matrix4 inverseView;
        Math::Matrix4 inverseProjection;
        Math::Matrix4 inverseViewProjection;
        Math::Matrix4 jitter;
        Math::Matrix4 viewProjectionJitter;
        // 16 byte boundary
        float nearPlane = 0.0f;
        float farPlane = 0.0f;
        float exposureValue100 = 0.0f;
        float foVH = 0.0f;
        // 16 byte boundary
        float foVV = 0.0f;
        float foVHTan = 0.0f;
        float foVVTan = 0.0f;
        float aspectRatio = 0;
        // 16 byte boundary
        Math::Vector4 front;
        // 16 byte boundary
        Math::Vector2 uvJitter;
        uint32_t pad1;
        uint32_t pad2;
        // 16 byte boundary
        Math::Vector4 planes[6];
    };

    struct GpuLightData {
    public:
        Math::Vector4 position;
        Math::Vector3 color;
        float         intensity;
        Math::Vector3 radiance;
        uint32_t      type;
    };

    /*
    * Item定义为每一个可渲染的实例
    */
    struct GpuItemData {
    public:
        Math::Matrix4 prevModelTrans;		        // 前一帧的世界变换矩阵
        Math::Matrix4 currModelTrans;		        // 当前帧的世界变换矩阵
        Math::BoundingBox boundingBoxInWorldSpace;  // 包围盒
        // ... 其他数据
    };

    /*
    * ItemGroup定义为多个形状相同的可渲染的实例的组合，其中itemDataBeginIndex标明了该组合中第一个实例的数据的其实位置
    * 所有的实例数据都存放在一个显存堆的ItemDataBuffer中(64KB对齐)
    */
    struct GpuItemGroupPassData {
    public:
        uint32_t itemVertexBufferIndex;   // 顶点缓存索引
        uint32_t itemIndexBufferIndex;    // 索引缓存索引
        uint32_t itemDataBeginIndex;      // Group中第一个实例的数据在ItemDataBuffer中起始位置
        float pad1;
    };

#pragma pack(1)
    struct GpuIndirectDrawData {
    public:
        D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;     // 当前帧数据
        D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;      // 当前阶段数据
        D3D12_GPU_VIRTUAL_ADDRESS lightDataAddress;     // 当前帧中的光照数据
        D3D12_DRAW_ARGUMENTS      drawArguments;        // 绘制参数
    };

#pragma pack (1)
    struct GpuIndirectDrawIndexedData {
    public:
        D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;      // 当前帧数据
        D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;       // 当前阶段数据
        D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;      // 当前帧中的光照数据
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;  // 绘制参数
    };

    struct GpuGTTonemappingParameters {
    public:
        float maximumLuminance = 270.0; // Your typical SDR monitor max luminance
        float contrast = 1.0;
        float linearSectionStart = 0.22;
        float linearSectionLength = 0.4;
        // 16 byte boundary
        float blackTightness = 1.33;
        float minimumBrightness = 0.0;
        float pad1;
        float pad2;
    };

}