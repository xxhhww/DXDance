#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <optional>
#include <numeric>
#include "Tools/Assert.h"

namespace Math {

	class Halton {
	public:
        template <uint32_t Dimensionality>
        static std::array<float, Dimensionality> Element(uint32_t elementIndex, std::optional<std::array<uint32_t, Dimensionality>> customBases = std::nullopt);

        template <uint32_t Dimensionality>
        static std::vector<std::array<float, Dimensionality>> Sequence(uint32_t elementStartIndex, uint32_t elementEndIndex);

        static uint32_t Prime(uint32_t n);

        static float Element(uint32_t elementIndex);

        static std::vector<float> Sequence(uint32_t elementStartIndex, uint32_t elementEndIndex);
	};

}

#include "Halton.inl"