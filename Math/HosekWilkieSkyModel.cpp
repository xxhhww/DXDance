#include "HosekWilkieSkyModel.h"
#include "Math/Common.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

namespace Math {

    double evaluate_spline(const double* spline, size_t stride, double value) {
        return 1 * pow(1 - value, 5) * spline[0 * stride] + 5 * pow(1 - value, 4) * pow(value, 1) * spline[1 * stride] + 10 * pow(1 - value, 3) * pow(value, 2) * spline[2 * stride] + 10 * pow(1 - value, 2) * pow(value, 3) * spline[3 * stride] + 5 * pow(1 - value, 1) * pow(value, 4) * spline[4 * stride] + 1 * pow(value, 5) * spline[5 * stride];
    }

    double evaluate(const double* dataset, size_t stride, float turbidity, float albedo, float sunTheta)
    {
        // splines are functions of elevation^1/3
        double elevationK = pow(std::max<float>(0.f, 1.f - sunTheta / (DirectX::XM_PI / 2.f)), 1.f / 3.0f);

        // table has values for turbidity 1..10
        int   turbidity0 = Math::Clamp(static_cast<int>(turbidity), 1, 10);
        int   turbidity1 = std::min(turbidity0 + 1, 10);
        float turbidityK = Math::Clamp(turbidity - turbidity0, 0.f, 1.f);

        const double* datasetA0 = dataset;
        const double* datasetA1 = dataset + stride * 6 * 10;

        double a0t0 = evaluate_spline(datasetA0 + stride * 6 * (turbidity0 - 1), stride, elevationK);
        double a1t0 = evaluate_spline(datasetA1 + stride * 6 * (turbidity0 - 1), stride, elevationK);
        double a0t1 = evaluate_spline(datasetA0 + stride * 6 * (turbidity1 - 1), stride, elevationK);
        double a1t1 = evaluate_spline(datasetA1 + stride * 6 * (turbidity1 - 1), stride, elevationK);

        return a0t0 * (1 - albedo) * (1 - turbidityK) + a1t0 * albedo * (1 - turbidityK) + a0t1 * (1 - albedo) * turbidityK + a1t1 * albedo * turbidityK;
    }

}