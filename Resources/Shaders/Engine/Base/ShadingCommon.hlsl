#ifndef _ShadingCommon__
#define _ShadingCommon__

#include "BRDFHelper.hlsl"
#include "Light.hlsl"

struct RandomSequences {
    float4 blueNoise;
    float4 halton;
};

struct ShadingResult {
    uint4  rayLightIntersectionData;
    float3 analyticUnshadowedOutgoingLuminance;
    float4 rayPDFs;
    float4x3 stochasticUnshadowedOutgoingLuminance;
};

float4 RandomNumbersForLight(RandomSequences randomSequences, uint rayIndex) {
    // Used for 2D position on light/BRDF
    float u1 = frac(randomSequences.halton.r + randomSequences.blueNoise.r);
    float u2 = frac(randomSequences.halton.g + randomSequences.blueNoise.g);

    // Choosing BRDF lobes
    float u3 = frac(randomSequences.halton.b + randomSequences.blueNoise.b);

    // Choosing between light and BRDF sampling
    float u4 = frac(randomSequences.halton.a + randomSequences.blueNoise.a);

    return float4(u1, u2, u3, u4);
}

/*
* Ì«Ñô¹â
*/
void ShadeWithSunLight(
    GBufferSurface  gBufferSurface,
    Light sun,
    RandomSequences randomSequences,
    float3 viewDirection,
    float3x3 surfaceWorldToTangent,
    inout ShadingResult shadingResult) {
    
    float3 sunDirection;
    float3 sunIlluminance;
    float3 sunLuminance;
    float sunDiskArea;
    float3 wi = mul(sunDirection, surfaceWorldToTangent);
    float3 wo = mul(viewDirection, surfaceWorldToTangent);
    float3 wm = normalize(wo + wi);
    float3 brdf = CookTorranceBRDF(wo, wi, wm, gBufferSurface);

    float pdf = 1.0f / sunDiskArea;
    shadingResult.analyticUnshadowedOutgoingLuminance += sunIlluminance * brdf;

    uint raysPerLight = 1u;
    float oneOverPDFOverRayCount = 1.0 / pdf / raysPerLight;

    [unroll]
    for (uint rayIdx = 0; rayIdx < raysPerLight; ++rayIdx) {
        float4 randomNumbers = RandomNumbersForLight(randomSequences, rayIdx);
        // shadingResult.rayLightIntersectionData[rayIdx] = PackRaySunIntersectionRandomNumbers(randomNumbers[rayIdx]);
        shadingResult.rayPDFs[rayIdx] = pdf;
        shadingResult.stochasticUnshadowedOutgoingLuminance[rayIdx] = brdf * sunLuminance * oneOverPDFOverRayCount;
    }
}

#endif