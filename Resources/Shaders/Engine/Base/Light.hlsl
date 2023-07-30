#ifndef _Light__
#define _Light__

#include "BRDFHelper.hlsl"
#include "../Math/MathCommon.hlsl"

#define MAX_NUM_SUN_SHADOW_CASCADES 4

struct Light {
	float4 position;	// sun direction + sunDiskArea
	float3 color;		// sun illuminance
	float  intensity;	
	float3 radiance;	// color * instensity
	uint   type;
};

struct LightContribution {
	float3 diffuse;
	float3 specular;

	float4 evaluate(float4 albedo) {
		float3 c = albedo.rgb * diffuse + specular;
		return float4(c, albedo.a);
	}

	void add(LightContribution other, float visibility = 1.0f) {
		diffuse  += other.diffuse * visibility;
		specular += other.specular * visibility;
	}

	void addSunLight(Surface surface, Light sunLight/*, float2 screenUV, float pixelDepth,
	Texture2D<float> shadowMap, SamplerComparisonState shadowSampler, float2 shadowMapTexelSize,
	Texture2D<float> sssTexture, SamplerState clampSampler, float subSurfaceScale = 0.f*/);
};

/*
计算直接光源
*/
static LightContribution calculateSunLighting(Surface surface, IntermediateInfo intermediateInfo, float specularScale = 1.0f) {
	float  D = distributionGGX(intermediateInfo.NdotH, surface.roughness);
	float  G = geometrySmith(intermediateInfo.NdotL, surface.NdotV, surface.roughness);
	float3 F = fresnelSchlick(intermediateInfo.VdotH, surface.F0);

	float3 kD = float3(1.f, 1.f, 1.f) - F;
	kD *= 1.f - surface.metallic;
	float3 diffuse = kD * M_INV_PI * intermediateInfo.radiance * intermediateInfo.NdotL;

	float3 specular = (D * G * F) / max(4.f * surface.NdotV, 0.001f) * intermediateInfo.radiance * specularScale;

	LightContribution result;
	result.diffuse  = diffuse;
	result.specular = specular;
	return result;
}

void LightContribution::addSunLight(Surface surface, Light sunLight /*, float2 screenUV, float pixelDepth,
	Texture2D<float> shadowMap, SamplerComparisonState shadowSampler, float2 shadowMapTexelSize,
	Texture2D<float> sssTexture, SamplerState clampSampler, float subSurfaceScale*/) {
	
	IntermediateInfo intermediateInfo;
	intermediateInfo.initialize(surface, sunLight.position.xyz, sunLight.radiance);

	add(calculateSunLighting(surface, intermediateInfo), 1.0f /*visibility*/);

	/*
	float visibility = sampleCascadedShadowMapPCF(lighting.sun.viewProjs, surface.P,
		shadowMap, lighting.sun.viewports,
		shadowSampler, lighting.shadowMapTexelSize, pixelDepth, lighting.sun.numShadowCascades,
		lighting.sun.cascadeDistances, lighting.sun.bias, lighting.sun.blendDistances);

	float sss = sssTexture.SampleLevel(clampSampler, screenUV, 0);
	visibility *= sss;
	*/

	/*
	[branch]
	if (visibility > 0.f) {
		add(calculateDirectLighting(surface, light), visibility);

		if (subSurfaceScale > 0.f) {
			// Subsurface scattering.
			// https://www.alanzucconi.com/2017/08/30/fast-subsurface-scattering-1/
			const float distortion = 0.4f;
			float3 sssH = L + surface.N * distortion;
			float sssVdotH = saturate(dot(surface.V, -sssH));

			float sssIntensity = sssVdotH * subSurfaceScale;
			diffuse += sssIntensity * lighting.sun.radiance * visibility;
		}
	}
	*/
}
#endif