#pragma once
#include <memory>
#include <vector>
#include "Math/Vector.h"

namespace Math {
    // A minimal adaptation of Spectrum code from PBRT v3
    extern float AverageSpectrumSamples(const float* lambda, const float* vals, int n, float lambdaStart, float lambdaEnd);

    inline void XYZToRGB(const Math::Vector3& xyz, Math::Vector3& rgb) {
        rgb.x = 3.240479f * xyz.x - 1.537150f * xyz.y - 0.498535f * xyz.z;
        rgb.y = -0.969256f * xyz.x + 1.875991f * xyz.y + 0.041556f * xyz.z;
        rgb.z = 0.055648f * xyz.x - 0.204043f * xyz.y + 1.057311f * xyz.z;
    }

    inline void RGBToXYZ(const Math::Vector3& rgb, Math::Vector3& xyz) {
        xyz.x = 0.412453f * rgb.x + 0.357580f * rgb.y + 0.180423f * rgb.z;
        xyz.y = 0.212671f * rgb.x + 0.715160f * rgb.y + 0.072169f * rgb.z;
        xyz.z = 0.019334f * rgb.x + 0.119193f * rgb.y + 0.950227f * rgb.z;
    }

    enum class SpectrumType {
        Reflectance, Illuminant
    };

    // Spectral Data Declarations
    static const int nCIESamples = 471;
    extern const float CIE_X[nCIESamples];
    extern const float CIE_Y[nCIESamples];
    extern const float CIE_Z[nCIESamples];
    extern const float CIE_lambda[nCIESamples];
    static const float CIE_Y_integral = 106.856895;
    static const int nRGB2SpectSamples = 32;
    extern const float RGB2SpectLambda[nRGB2SpectSamples];
    extern const float RGBRefl2SpectWhite[nRGB2SpectSamples];
    extern const float RGBRefl2SpectCyan[nRGB2SpectSamples];
    extern const float RGBRefl2SpectMagenta[nRGB2SpectSamples];
    extern const float RGBRefl2SpectYellow[nRGB2SpectSamples];
    extern const float RGBRefl2SpectRed[nRGB2SpectSamples];
    extern const float RGBRefl2SpectGreen[nRGB2SpectSamples];
    extern const float RGBRefl2SpectBlue[nRGB2SpectSamples];
    extern const float RGBIllum2SpectWhite[nRGB2SpectSamples];
    extern const float RGBIllum2SpectCyan[nRGB2SpectSamples];
    extern const float RGBIllum2SpectMagenta[nRGB2SpectSamples];
    extern const float RGBIllum2SpectYellow[nRGB2SpectSamples];
    extern const float RGBIllum2SpectRed[nRGB2SpectSamples];
    extern const float RGBIllum2SpectGreen[nRGB2SpectSamples];
    extern const float RGBIllum2SpectBlue[nRGB2SpectSamples];

    class CoefficientSpectrum {
    public:
        CoefficientSpectrum(uint32_t sampleCount);

        CoefficientSpectrum& operator+=(const CoefficientSpectrum& s2);
        CoefficientSpectrum operator+(const CoefficientSpectrum& s2) const;
        CoefficientSpectrum operator-(const CoefficientSpectrum& s2) const;
        CoefficientSpectrum operator/(const CoefficientSpectrum& s2) const;
        CoefficientSpectrum operator*(const CoefficientSpectrum& sp) const;
        CoefficientSpectrum& operator*=(const CoefficientSpectrum& sp);
        CoefficientSpectrum operator*(float a) const;
        CoefficientSpectrum& operator*=(float a);
        CoefficientSpectrum operator/(float a) const;
        CoefficientSpectrum& operator/=(float a);
        CoefficientSpectrum operator-() const;
        bool operator==(const CoefficientSpectrum& sp) const;
        bool operator!=(const CoefficientSpectrum& sp) const;
        float& operator[](int i);
        float operator[](int i) const;
        bool IsBlack() const;
        float MaxComponentValue() const;

        CoefficientSpectrum Clamp(float low = 0, float high = std::numeric_limits<float>::infinity()) const;

        friend inline CoefficientSpectrum operator*(float a, const CoefficientSpectrum& s);

    protected:
        std::vector<float> mSamples;

    public:
        inline const auto& GetSamples() const { return mSamples; }
    };

    class SampledSpectrum : public CoefficientSpectrum {
    public:
        SampledSpectrum(uint32_t sampleCount, float lowestWavelength, float highestWavelength);
        SampledSpectrum(const CoefficientSpectrum& v, float lowestWavelength, float highestWavelength);

        Math::Vector3 ToXYZ() const;
        Math::Vector3 ToRGB() const;

        float Y() const;

        void Init();
        void FromRGB(const Math::Vector3& rgb, SpectrumType type = SpectrumType::Illuminant);
        void FromXYZ(const Math::Vector3& xyz, SpectrumType type = SpectrumType::Reflectance);

    private:
        using CoefficientSpectrum::mSamples;

        float mLowestWavelength;
        float mHighestWavelength;

        std::unique_ptr<SampledSpectrum> mX;
        std::unique_ptr<SampledSpectrum> mY;
        std::unique_ptr<SampledSpectrum> mZ;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectWhite;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectCyan;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectMagenta;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectYellow;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectRed;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectGreen;
        std::unique_ptr<SampledSpectrum> mRGBRefl2SpectBlue;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectWhite;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectCyan;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectMagenta;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectYellow;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectRed;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectGreen;
        std::unique_ptr<SampledSpectrum> mRGBIllum2SpectBlue;

    public:
        inline float LowestWavelength() const { return mLowestWavelength; }
        inline float HighestWavelength() const { return mHighestWavelength; }
    };
}