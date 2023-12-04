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

ShadingResult ZeroShadingResult() {
    ShadingResult result;
    result.rayLightIntersectionData = 0;
    result.analyticUnshadowedOutgoingLuminance = 0.0;
    result.rayPDFs = 0.0;
    result.stochasticUnshadowedOutgoingLuminance = 0.0;
    return result;
}

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
* 太阳光
*/
void ShadeWithSunLight(
    GBufferSurface  gBufferSurface,
    Light sun,
    RandomSequences randomSequences,
    float3 viewDirection,
    inout ShadingResult shadingResult) {
    
    float3 sunDirection = sun.position.xyz;
    float  sunDiskArea = sun.position.w;
    float3 sunIlluminance = sun.color;
    // float3 sunLuminance = float3(0.0f, 0.0f, 0.0f);

    float3 wi = sunDirection;
    float3 wo = viewDirection;
    float3 wm = normalize(wo + wi);
    float3 brdf = CookTorranceBRDF(wo, wi, wm, gBufferSurface);

    // float pdf = 1.0f / sunDiskArea;
    shadingResult.analyticUnshadowedOutgoingLuminance += (sunIlluminance * brdf + 0.03f);

    /*
    uint raysPerLight = 1u;
    float oneOverPDFOverRayCount = 1.0 / pdf / raysPerLight;
    [unroll]
    for (uint rayIdx = 0; rayIdx < raysPerLight; ++rayIdx) {
        float4 randomNumbers = RandomNumbersForLight(randomSequences, rayIdx);
        shadingResult.rayLightIntersectionData[rayIdx] = PackRaySunIntersectionRandomNumbers(randomNumbers[rayIdx]);
        shadingResult.rayPDFs[rayIdx] = pdf;
        shadingResult.stochasticUnshadowedOutgoingLuminance[rayIdx] = brdf * sunLuminance * oneOverPDFOverRayCount;
    }
    */
}

/*
* 延迟管线中获取CSM Shadow
*/
float CalculateCSMInDeferredPipeline(float3 wsPosition, float4x4 mvp, float shadowTextureSize, uint cascadedShadowTextureIndex, in SamplerComparisonState samplerCascadedPcfShadowMap) {
    Texture2D<float> cascadedShadowTexture = ResourceDescriptorHeap[cascadedShadowTextureIndex];

    // 转换到光源空间(太阳光)
    float4 lightSpacePos = mul(float4(wsPosition, 1.0f), mvp);
    float4 shadowCoord = lightSpacePos / lightSpacePos.w;
    shadowCoord.rg = shadowCoord.rg * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    
    const float dilation = 2.0;
    float d1 = dilation * shadowTextureSize * 0.125;
    float d2 = dilation * shadowTextureSize * 0.875;
    float d3 = dilation * shadowTextureSize * 0.625;
    float d4 = dilation * shadowTextureSize * 0.375;
    float result = (
        2.0 * cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy, shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(-d2, d1),  shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(-d1, -d2), shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(d2, -d1),  shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(d1, d2),   shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(-d4, d3),  shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(-d3, -d4), shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(d4, -d3),  shadowCoord.z) +
        cascadedShadowTexture.SampleCmpLevelZero(samplerCascadedPcfShadowMap, shadowCoord.xy + float2(d3, d4),   shadowCoord.z)
        ) / 10.0;

    return result * result;
}

#endif