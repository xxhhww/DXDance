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
    * Item����Ϊÿһ������Ⱦ��ʵ��
    */
    struct GpuItemData {
    public:
        Math::Matrix4 prevModelTrans;		        // ǰһ֡������任����
        Math::Matrix4 currModelTrans;		        // ��ǰ֡������任����
        Math::BoundingBox boundingBoxInWorldSpace;  // ��Χ��
        // ... ��������
    };

    /*
    * ItemGroup����Ϊ�����״��ͬ�Ŀ���Ⱦ��ʵ������ϣ�����itemDataBeginIndex�����˸�����е�һ��ʵ�������ݵ���ʵλ��
    * ���е�ʵ�����ݶ������һ���Դ�ѵ�ItemDataBuffer��(64KB����)
    */
    struct GpuItemGroupPassData {
    public:
        uint32_t itemVertexBufferIndex;   // ���㻺������
        uint32_t itemIndexBufferIndex;    // ������������
        uint32_t itemDataBeginIndex;      // Group�е�һ��ʵ����������ItemDataBuffer����ʼλ��
        float pad1;
    };

#pragma pack(1)
    struct GpuIndirectDrawData {
    public:
        D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;     // ��ǰ֡����
        D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;      // ��ǰ�׶�����
        D3D12_GPU_VIRTUAL_ADDRESS lightDataAddress;     // ��ǰ֡�еĹ�������
        D3D12_DRAW_ARGUMENTS      drawArguments;        // ���Ʋ���
    };

#pragma pack (1)
    struct GpuIndirectDrawIndexedData {
    public:
        D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;      // ��ǰ֡����
        D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;       // ��ǰ�׶�����
        D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;      // ��ǰ֡�еĹ�������
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;  // ���Ʋ���
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