#pragma once

namespace Math {

    double evaluate_spline(const double* spline, size_t stride, double value);

    double evaluate(const double* dataset, size_t stride, float turbidity, float albedo, float sunTheta);

}