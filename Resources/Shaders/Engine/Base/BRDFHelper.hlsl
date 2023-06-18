#ifndef _BRDFHelper__
#define _BRDFHelper__

#include "GBufferHelper.hlsl"
#include "../Math/Trig.hlsl"
#include "../Math/MathCommon.hlsl"

// Following Wo, Wi, Wm are vectors in tangent space of the surface 

// The Fresnel equation describes the ratio of surface reflection at different surface angles.
// (This is an approximation of Fresnels' equation, called Fresnel-Schlick)
// Describes the ratio of light that gets reflected over the light that gets refracted,
// which varies over the angle we're looking at a surface.
//
// f0 - reflection coefficient for light incoming parallel to the normal
// (i.e., the value of the Fresnel term when  = 0 or minimal reflection)
//
// f90 - reflection coefficient for light incoming perpendicular to the normal
// (should be 1 in realistic scenario, but left as a variable rather then a constant to add flexibility)
//
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
//
float3 FresnelSchlick(float3 f0, float3 f90, float cosPhi) {
    // Clamp between 0 and 1, because input cos might be slightly greater 
    // than 1 due to accumulated errors which will leads pow to output NaNs
    return f0 + (f90 - f0) * pow(saturate(1.0f - cosPhi), 5.0f); 
}

// Trowbridge-Reitz GGX normal distribution function
// Statistically approximates the ratio of microfacets aligned to some (halfway) vector h
//
float NormalDistributionTrowbridgeReitzGGXIsotropic(float3 wm, float alpha) {
    float a2 = alpha * alpha;
    float NdotH2 = Cos2Theta(wm);

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

float NormalDistributionTrowbridgeReitzGGXAnisotropic(float3 wm, float ax, float ay) {
    float dotHX2 = Square(wm.x);
    float dotHY2 = Square(wm.y);
    float cos2Theta = Cos2Theta(wm);
    float ax2 = Square(ax);
    float ay2 = Square(ay);

    return 1.0f / (PI * ax * ay * Square(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta));
}

// Statistically approximates the ratio of microfacets that overshadow each other causing light rays to lose their energy in the process.
// https://schuttejoe.github.io/

// G1 and G2 naming note: 
// G2(L, V, M) is % visible in 2 directions
// G1(V, M) is % visible in just 1 direction
// In practice, G2 is derived from G1: G2(L, V, M) = G1(L, M) * G1(V, M)
// From https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf, slide 21

// Note on G2 meaning: to effectively approximate the geometry we need to take account 
// of both the view direction (geometry obstruction) and the light direction vector (geometry shadowing).
float HeightCorrelatedSmithGGXG2(float3 wo, float3 wi, float a) {
    float absDotNV = AbsCosTheta(wo);
    float absDotNL = AbsCosTheta(wi);
    float a2 = a * a;

    // height-correlated masking function
    // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
    float denomA = absDotNV * sqrt(a2 + (1.0f - a2) * absDotNL * absDotNL);
    float denomB = absDotNL * sqrt(a2 + (1.0f - a2) * absDotNV * absDotNV);

    return 2.0f * absDotNL * absDotNV / (denomA + denomB + 0.001); // add constant to avoid NaNs
}

float SeparableSmithGGXG1(float3 w, float3 wm, float ax, float ay) {
    float absTanTheta = abs(TanTheta(w));

    if (isinf(absTanTheta))
        return 0.0f;

    float a = sqrt(Cos2Phi(w) * ax * ax + Sin2Phi(w) * ay * ay);
    float a2Tan2Theta = Square(a * absTanTheta);

    float lambda = 0.5f * (-1.0f + sqrt(1.0f + a2Tan2Theta));
    return 1.0f / (1.0f + lambda);
}

float SeparableSmithGGXG1(float3 w, float a) {
    float a2 = a * a;
    float absDotNV = AbsCosTheta(w);

    return 2.0f / (1.0f + sqrt(a2 + (1.0f - a2) * absDotNV * absDotNV));
}

// Renormalized Disney diffuse formula to be energy conserving 
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// 3.1.3 Energy conservation
//
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness) {
    float energyBias = lerp(0.0f, 0.5f, linearRoughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
    float fd90 = energyBias + 2.0f * LdotH * LdotH * linearRoughness;
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float lightScatter = FresnelSchlick(f0, fd90, NdotL).r;
    float viewScatter = FresnelSchlick(f0, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor / PI;
}

float3 DiffuseBRDFForGI(float3 viewDirection, GBufferSurface surface) {
    // Since Disney diffuse if not uniform in all directions like Lambert,
    // ideally we would precompute irradiance with Disney diffuse term computed for each ray,
    // but instead we do it a bit hacky and apply single term to whole irradiance
    float3 L = surface.normal;
    float3 H = normalize(L + viewDirection);
    float NdotV = dot(surface.normal, viewDirection);
    float LdotH = dot(H, L);
    float NdotH = LdotH;

    float3 f0 = lerp(0.04f, surface.albedo, surface.metalness);
    float3 F = FresnelSchlick(f0, 1.0f, NdotH);
    float3 diffuse = DisneyDiffuse(NdotV, 1.0f, LdotH, surface.roughness) * (1.0f - surface.metalness) * surface.albedo * (1.0f - F);
    
    return diffuse;
}

float3 CookTorranceBRDF(float3 wo, float3 wi, float3 wm, GBufferSurface surface) {
    // Based on observations by Disney and adopted by Epic Games
    // the lighting looks more correct squaring the roughness
    // in both the geometry and normal distribution function.
    float roughness2 = surface.roughness * surface.roughness;
    float NdotL = saturate(CosTheta(wi));
    float NdotV = saturate(CosTheta(wo));
    float NdotH = saturate(CosTheta(wm));

    float NDF = NormalDistributionTrowbridgeReitzGGXIsotropic(wm, roughness2);
    float G = HeightCorrelatedSmithGGXG2(wo, wi, roughness2);

    const float BaseDielectricReflectivity = 0.04f;

    float3 f0 = lerp(BaseDielectricReflectivity, surface.albedo, surface.metalness);
    float3 F = FresnelSchlick(f0, 1.0f, NdotH);

    // Small denom will blow specular term beyond any reasonable values
    float specularDenom = 4.0f * NdotL * NdotV;
    float3 specular = specularDenom > 0.0001f ? ((NDF * G * F) / specularDenom) : 0.0f;

    float3 diffuseFactor = (1.0f - F) * (1.0f - surface.metalness) * surface.albedo;
    float3 diffuse = DisneyDiffuse(NdotV, NdotL, NdotH, surface.roughness) * diffuseFactor;

    return (diffuse + specular) * NdotL;
}

#endif