#ifndef _ToneMappingPass__
#define _ToneMappingPass__

#include "ToneMappingHelper.hlsl"
#include "../Base/Exposure.hlsl"
#include "../Base/ColorSpaceHelper.hlsl"

struct PassData {
    uint  inputMapIndex;
    uint  outputMapIndex;
    bool  isHDREnabled;
    float displayMaxLuminance;
    GTTonemappingParams tonemappingParams;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int groupIndex : SV_GroupIndex) {
    Texture2D<float4>   inputImage = ResourceDescriptorHeap[PassDataCB.inputMapIndex];
    RWTexture2D<float4> outputImage = ResourceDescriptorHeap[PassDataCB.outputMapIndex];

    uint2 pixelIndex = dispatchThreadID.xy;

    float3 color = inputImage[pixelIndex].rgb;
    // color = ExposeLuminance(color, FrameDataCB.CurrentEditorCamera);

    GTTonemappingParams params = PassDataCB.tonemappingParams;
    // Luminance was exposed using Saturation Based Sensitivity method 
    // hence the 1.0 for maximum luminance
    params.MaximumLuminance = 1.0;

    // NaN detection for convenience
    if (any(isnan(color))) {
        color = float3(0, 10, 0);
        outputImage[pixelIndex] = float4(color, 1.0);
        return;
    }

    color = float3(
        GTToneMap(color.r, params),
        GTToneMap(color.g, params),
        GTToneMap(color.b, params));

    /*
    // Histogram computation
    if (groupIndex < HistogramBinCount) {
        gHistogram[groupIndex] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    float2 minMaxLum = float2(0.0, 1.0);
    uint bin = GetHistogramBin(CIELuminance(color), minMaxLum.x, minMaxLum.y);
    uint prevValue;
    InterlockedAdd(gHistogram[bin], 1, prevValue);
    GroupMemoryBarrierWithGroupSync();

    if (groupIndex < HistogramBinCount) {
        InterlockedAdd(Histogram[groupIndex], gHistogram[groupIndex], prevValue);
    }
    */

    // Color gamut and quantizer conversions
    if (PassDataCB.isHDREnabled) {
        // Remap tone mapped 1.0 value to correspond to maximum luminance of the display
        const float ST2084Max = 10000.0;
        const float HDRScalar = PassDataCB.displayMaxLuminance / ST2084Max;

        // The HDR scene is in Rec.709, but the display is Rec.2020
        color = Rec709ToRec2020(color);

        // Apply the ST.2084 curve to the scene.
        color = LinearToST2084(color * HDRScalar);
    }
    else {
        color = LinearToSRGB(color);
    }

    outputImage[pixelIndex] = float4(color, 1.0);
}

#endif