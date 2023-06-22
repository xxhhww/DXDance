#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <array>

namespace Renderer {

    struct GPUCamera {
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
        uint32_t pad1;
        uint32_t pad2;
        // 16 byte boundary
        Math::Vector4 planes[6];
    };

    struct GPULight {
        Math::Vector4 position;
        Math::Vector3 color;
        uint32_t type;
    };

    struct GPUArHosekSkyModelState {
        std::array<std::array<float, 12>, 3> configs;
        Math::Vector4 radiances;
    };

    struct GPUGTTonemappingParameters {
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