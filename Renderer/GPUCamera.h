#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Renderer {

    struct GPUCamera
    {
        Math::Vector4 position;
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
        uint32_t pad0__;
        uint32_t pad1__;
    };

}