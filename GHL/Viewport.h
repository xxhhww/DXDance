#pragma once
#include "pbh.h"

namespace GHL {

	class Viewport {
    public:
        Viewport(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height, float minDepth, float maxDepth);
        Viewport(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height);
        Viewport(uint16_t width, uint16_t height);

        D3D12_VIEWPORT D3DViewport() const;

	public:
        uint16_t topLeftX;
        uint16_t topLeftY;
        uint16_t width;
        uint16_t height;
        float minDepth;
        float maxDepth;
	};

}