#ifndef _SkyGenerationPass__
#define _SkyGenerationPass__

struct ArHosekSkyModelState {
    float4 configsX[3];
    float4 configsY[3];
    float4 configsZ[3];
    float4 radiances;
};

struct PassData{
    float3 sunDirection;
    uint   skyLuminanceMapIndex;
    uint2   skyLuminanceMapSize;
    uint2  dispatchGroupCount;
    ArHosekSkyModelState skyStateR; 
    ArHosekSkyModelState skyStateG;
    ArHosekSkyModelState skyStateB;
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

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
    RWTexture2D<float4> skyLuminanceMap = ResourceDescriptorHeap[PassDataCB.skyLuminanceMapIndex];

    // uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, groupThreadID.xy, groupID.xy);
    uint2 pixelIndex = dispatchThreadID.xy;
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.skyLuminanceMapSize);

    float2 octVector = uv * 2.0 - 1.0;
    float3 sampleVector = OctDecode(octVector);
    
    float gamma = acos(dot(sampleVector, PassDataCB.sunDirection));
    // Clamp because hosek code is not robust against edge cases 
    float theta = acos(clamp(sampleVector.y, 0.0001, 0.9999));

    float r = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateR.configsX, theta, gamma) * PassDataCB.skyStateR.radiances[0];
    float g = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateG.configsY, theta, gamma) * PassDataCB.skyStateG.radiances[1];
    float b = ArHosekSkyModel_GetRadianceInternal(PassDataCB.skyStateB.configsZ, theta, gamma) * PassDataCB.skyStateB.radiances[2];

    float3 rgb = float3(r, g, b);
    rgb *= StandardLuminousEfficacy;

    skyLuminanceMap[pixelIndex].rgb = rgb;

	return;
}

#endif