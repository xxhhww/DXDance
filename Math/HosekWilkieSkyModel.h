#pragma once
#include "Math/Vector.h"
#include <array>

namespace Math {

	enum ESkyParams : uint16_t {
		ESkyParam_A = 0,
		ESkyParam_B,
		ESkyParam_C,
		ESkyParam_D,
		ESkyParam_E,
		ESkyParam_F,
		ESkyParam_G,
		ESkyParam_I,
		ESkyParam_H,
		ESkyParam_Z,
		ESkyParam_Count
	};

    using SkyParameters = std::array<Math::Vector3, std::underlying_type<ESkyParams>::type(ESkyParams::ESkyParam_Count)>;

    class HosekWilkieSkyModel {
    public:
        static SkyParameters CalculateSkyParameters(float turbidity, float albedo, Math::Vector3 sun_direction);

    private:
		static double EvaluateSpline(double const* spline, size_t stride, double value);

        static double Evaluate(double const* dataset, size_t stride, float turbidity, float albedo, float sun_theta);
    
		static Math::Vector3 HosekWilkie(float cos_theta, float gamma, float cos_gamma, Math::Vector3 A, Math::Vector3 B, Math::Vector3 C, Math::Vector3 D, Math::Vector3 E, Math::Vector3 F, Math::Vector3 G, Math::Vector3 H, Math::Vector3 I);
	};

}