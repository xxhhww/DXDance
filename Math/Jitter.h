#pragma once
#include <array>
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Math {

	class Jitter {
	public:
		Math::Matrix4 jitterMatrix;
		Math::Vector2 uvJitter;

		static Jitter GetJitter(uint64_t frameIndex, uint64_t maxJitterFrames, const Math::Vector2& frameResolution);

	private:
		inline static std::array<Math::Vector2, 16> smJitterHaltonSamples = {
			Math::Vector2{ 0.000000 , -0.166667 },
			Math::Vector2{ -0.250000,  0.166667 },
			Math::Vector2{ 0.250000 , -0.388889 },
			Math::Vector2{ -0.375000, -0.055556 },
			Math::Vector2{ 0.125000 ,  0.277778 },
			Math::Vector2{ -0.125000, -0.277778 },
			Math::Vector2{ 0.375000 ,  0.055556 },
			Math::Vector2{ -0.437500,  0.388889 },
			Math::Vector2{ 0.062500 , -0.462963 },
			Math::Vector2{ -0.187500, -0.129630 },
			Math::Vector2{ 0.312500 ,  0.203704 },
			Math::Vector2{ -0.312500, -0.351852 },
			Math::Vector2{ 0.187500 , -0.018519 },
			Math::Vector2{ -0.062500,  0.314815 },
			Math::Vector2{ 0.437500 , -0.240741 },
			Math::Vector2{ -0.468750,  0.092593 },
		};
	};

}