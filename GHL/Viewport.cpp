#include "Viewport.h"

namespace GHL {

    Viewport::Viewport(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height, float minDepth, float maxDepth) 
    : topLeftX(topLeftX)
    , topLeftY(topLeftY)
    , width(width)
    , height(height)
    , minDepth(minDepth)
    , maxDepth(maxDepth) {
    }
    
    Viewport::Viewport(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height) 
    : topLeftX(topLeftX)
    , topLeftY(topLeftY)
    , width(width)
    , height(height)
    , minDepth(0.0f)
    , maxDepth(1.0f) {
    }
    
    Viewport::Viewport(uint16_t width, uint16_t height) 
    : topLeftX(0u)
    , topLeftY(0u)
    , width(width)
    , height(height)
    , minDepth(0.0f)
    , maxDepth(1.0f) {
    }

    D3D12_VIEWPORT Viewport::D3DViewport() const {
        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = topLeftX;
        viewport.TopLeftY = topLeftY;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = minDepth;
        viewport.MaxDepth = maxDepth;

        return viewport;
    }

}