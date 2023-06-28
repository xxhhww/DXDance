#ifndef _SkyGenerationPass__
#define _SkyGenerationPass__

struct HosekSkyModelState {
    float4 A;
    float4 B;
    float4 C;
    float4 D;
    float4 E;
    float4 F;
    float4 G;
    float4 H;
    float4 I;
    float4 Z;
};

struct PassData{
    float3 sunDirection;
    uint   skyLuminanceMapIndex;
    uint2  skyLuminanceMapSize;
    uint2  dispatchGroupCount;
    HosekSkyModelState skyParameters; 
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"
#include "Base/Utils.hlsl"
#include "Math/MathCommon.hlsl"


float ArHosekSkyModel_GetRadianceInternal(float4 config[3], float theta, float gamma) {
    float configuration[9] = (float[9])config;

    const float expM = exp(configuration[4] * gamma);
    const float rayM = cos(gamma) * cos(gamma);
    const float mieM = (1.0 + cos(gamma) * cos(gamma)) / pow((1.0 + configuration[8] * configuration[8] - 2.0 * configuration[8] * cos(gamma)), 1.5);
    const float zenith = sqrt(cos(theta));

    return (1.0 + configuration[0] * exp(configuration[1] / (cos(theta) + 0.01))) *
        (configuration[2] + configuration[3] * expM + configuration[5] * rayM + configuration[6] * mieM + configuration[7] * zenith);
}

float3 HosekWilkie(float cosTheta, float gamma, float cosGamma) {
	const float3 A = PassDataCB.skyParameters.A.xyz;
	const float3 B = PassDataCB.skyParameters.B.xyz;
	const float3 C = PassDataCB.skyParameters.C.xyz;
	const float3 D = PassDataCB.skyParameters.D.xyz;
	const float3 E = PassDataCB.skyParameters.E.xyz;
	const float3 F = PassDataCB.skyParameters.F.xyz;
	const float3 G = PassDataCB.skyParameters.G.xyz;
	const float3 H = PassDataCB.skyParameters.H.xyz;
	const float3 I = PassDataCB.skyParameters.I.xyz;

	float3 chi = (1 + cosGamma * cosGamma) / pow(1 + H * H - 2 * cosGamma * H, float3(1.5f, 1.5f, 1.5f));
	return (1 + A * exp(B / (cosTheta + 0.01))) * (C + D * exp(E * gamma) + F * (cosGamma * cosGamma) + G * chi + I * sqrt(cosTheta));
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
    RWTexture2D<float4> skyLuminanceMap = ResourceDescriptorHeap[PassDataCB.skyLuminanceMapIndex];

    // uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.skyLuminanceMapSize);

    float2 octVector = uv * 2.0f - 1.0f;
    float3 sampleVector = OctDecode(octVector);     // 采样向量
    float3 sunDirection = PassDataCB.sunDirection;

    float cosTheta = clamp(sampleVector.y, 0.0001f, 1.0f);
	float cosGamma = clamp(dot(sampleVector, sunDirection), 0.0001f, 1.0f);
	float gamma = acos(cosGamma);

    float3 rgb = -PassDataCB.skyParameters.Z.xyz * HosekWilkie(cosTheta, gamma, cosGamma);
    /*
    sunDirection.y = clamp(sunDirection.y, 0.0001f, 1.0f);
    float gamma = acos(clamp(dot(sampleVector, sunDirection), 0.0001f, 1.0f));
    // Clamp because hosek code is not robust against edge cases 
    float theta = acos(clamp(sampleVector.y, 0.0001f, 1.0f));

    // 通过反三角函数，获得sun的Theta角度
    float thetaS = acos(clamp(sunDirection.y, 0.0001f, 1.0f));
    float tempR = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateR.configsX, thetaS, 0.0f) * PassDataCB.skyStateR.radiances[0];
    float tempG = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateG.configsY, thetaS, 0.0f) * PassDataCB.skyStateG.radiances[1];
    float tempB = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateB.configsZ, thetaS, 0.0f) * PassDataCB.skyStateB.radiances[2];
    float temp  = dot(float3(tempR, tempG, tempB), float3(0.2126f, 0.7152f, 0.0722f));
    float3 z = PassDataCB.skyStateR.radiances.xyz / temp;
    z *= 0.75f;

    float r = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateR.configsX, theta, gamma) * z.x;
    float g = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateG.configsY, theta, gamma) * z.y;
    float b = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateB.configsZ, theta, gamma) * z.z;

    float3 rgb = float3(r, g, b);
    */
    skyLuminanceMap[pixelIndex].rgb = rgb;

	return;
}

#endif