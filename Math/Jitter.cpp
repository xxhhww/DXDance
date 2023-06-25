#include "Jitter.h"
#include "Math/Common.h"

namespace Math {

	Jitter Jitter::GetJitter(uint64_t frameIndex, uint64_t maxJitterFrames, const Math::Vector2& frameResolution) {
        Jitter jitter{};

        maxJitterFrames = Math::Max(maxJitterFrames, smJitterHaltonSamples.size());
        frameIndex = frameIndex % maxJitterFrames;

        Math::Vector2 texelSizeInNDC = 2.0f / Math::Vector2{ frameResolution };
        Math::Matrix4 jitterMatrix{};

        // Halton sequence samples are in [-0.5, 0.5] range
        jitterMatrix(3, 0) = texelSizeInNDC.x * smJitterHaltonSamples[frameIndex].x;
        jitterMatrix(3, 1) = texelSizeInNDC.y * smJitterHaltonSamples[frameIndex].y;

        jitter.jitterMatrix = jitterMatrix;
        // Jitter is in NDC space, so we need to divide it by 2 to account for UV space being 2 times smaller
        // Also Y component is inverted because NDC up and UV up are opposite
        jitter.uvJitter = { jitterMatrix(3, 0) * 0.5f, jitterMatrix(3, 1) * -0.5f};

        return jitter;
	}

}