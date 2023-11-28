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
        float foVV = 0.0f;      // 规定fovH与fovY含义相同
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

    public:
        GpuItemData() = default;
        ~GpuItemData() = default;
    };

    struct GpuItemIndirectDrawIndexedData {
    public:
        D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;      // 当前帧数据
        D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;       // 当前阶段数据
        D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;      // 当前帧中的光照数据
        D3D12_GPU_VIRTUAL_ADDRESS    itemDataAddress;       // 当前Item数据
        D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;      // 顶点
        D3D12_INDEX_BUFFER_VIEW      indexBufferView;       // 索引
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;  // 绘制参数
        float pad1;
        float pad2;
        float pad3;

    public:
        GpuItemIndirectDrawIndexedData() = default;
        ~GpuItemIndirectDrawIndexedData() = default;
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