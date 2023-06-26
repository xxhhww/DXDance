#ifndef _TAAPass__
#define _TAAPass__

#include "TAAHelper.hlsl"

struct PassData {
    uint2 dispatchGroupCount;
    uint  previousTAAOutputMapIndex;    // 上一帧TAA的输出结果
    uint  previousPassOutputMapIndex;   // 前一个Pass的输出结果
    uint  gBufferMotionVectorMapIndex;  // GBufferMotionVector
    uint  currentTAAOutputMapIndex;     // 当前帧TAA的输出结果
    uint  isFirstFrame;
    float pad1;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

groupshared min16float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

void LoadNeighbors(uint2 pixelIndex, int2 groupThreadID, Texture2D<float4> image) {
    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(pixelIndex, groupThreadID, FrameDataCB.FinalRTResolution, GroupDimensionSize, SamplingRadius);

    float3 centerColor = ConvertToWorkingSpace(image[coords.LoadCoord0].rgb);

    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = min16float3(centerColor);

    if (coords.IsLoadStore1Required) {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord1].rgb);
        gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = min16float3(color);
    }

    if (coords.IsLoadStore2Required) {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord2].rgb);
        gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = min16float3(color);
    }

    if (coords.IsLoadStore3Required) {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord3].rgb);
        gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = min16float3(color);
    }

    GroupMemoryBarrierWithGroupSync();
}

float3 SampleHistory(Texture2D<float4> historyTex, float2 uv) {
    float3 history = historyTex.SampleLevel(SamplerLinearClamp, uv, 0.0).rgb;
    history = ConvertToWorkingSpace(history);
    return history;
}

AABB GetVarianceAABB(int2 GTindex, float3 center, float stDevMultiplier) {
    float3 M1 = center; // First moment - Average
    float3 M2 = sqrt(center); // Second moment - Variance
    float sampleCount = 1.0;

    [unroll] 
    for (int x = -SamplingRadius; x <= SamplingRadius; ++x) {
        [unroll]
        for (int y = -SamplingRadius; y <= SamplingRadius; ++y) {
            if (x == 0 && y == 0)
                continue;

            int2 loadCoord = GTindex + int2(x, y);
            float3 color = gCache[loadCoord.x][loadCoord.y];

            M1 += color;
            M2 += sqrt(color);
            sampleCount += 1.0;
        }
    }

    float3 MU = M1 / sampleCount;
    float3 sigma = sqrt(max(M2 / sampleCount - MU * MU, 0.0));

    return InitAABB(MU - stDevMultiplier * sigma, MU + stDevMultiplier * sigma);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
    Texture2D<float4>   previousTAAOutputMap   = ResourceDescriptorHeap[PassDataCB.previousTAAOutputMapIndex];
    Texture2D<float4>   previousPassOutputMap  = ResourceDescriptorHeap[PassDataCB.previousPassOutputMapIndex];
    Texture2D<float4>   gBufferMotionVectorMap = ResourceDescriptorHeap[PassDataCB.gBufferMotionVectorMapIndex];
    RWTexture2D<float4> currentTAAOutputMap    = ResourceDescriptorHeap[PassDataCB.currentTAAOutputMapIndex];

    uint2  pixelIndex = dispatchThreadID.xy;
    float2 pixelUV = TexelIndexToUV(pixelIndex, FrameDataCB.FinalRTResolution);
    float3 motionVector = gBufferMotionVectorMap[pixelIndex].xyz;
    float2 reprojectedUV = pixelUV - motionVector.xy; // 映射到上一帧的UV

    // 当前帧为第一帧
    if (PassDataCB.isFirstFrame) {
        currentTAAOutputMap[pixelIndex].rgba = previousPassOutputMap[pixelIndex].rgba;
        return;
    }

    LoadNeighbors(pixelIndex, groupThreadID.xy, previousPassOutputMap);

    // Variance clipping
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    // https://en.wikipedia.org/wiki/Moment_(mathematics)
    int2 groupThreadIndex = groupThreadID.xy + SamplingRadius;
    float3 center = gCache[groupThreadIndex.x][groupThreadIndex.y];
    float3 history = SampleHistory(previousTAAOutputMap, reprojectedUV);
    float colorLuma = GetLuminance(center);
    float historyLuma = GetLuminance(history);

    float stDevMultiplier = 1.0f;

    // The reasoning behind the anti flicker is that if we have high spatial contrast (high standard deviation)
    // and high temporal contrast, we let the history to be closer to be unclipped. To achieve, the min/max bounds
    // are extended artificially more.
    float temporalContrast = saturate(abs(colorLuma - historyLuma) / max(0.2f, max(colorLuma, historyLuma)));
     
    float motionVectorLen = length(motionVector.xy);
    float screenDiag = length(float2(FrameDataCB.FinalRTResolution));
    const float MaxFactorScale = 2.25f; // when stationary
    const float MinFactorScale = 0.8f; // when moving more than slightly
    // Anti-flicker factor is shrunk under motion
    float localizedAntiFlicker = lerp(MinFactorScale, MaxFactorScale, saturate(1.0f - 2.0f * (motionVectorLen * screenDiag)));

    // Extend AABB if temporal contrast is high
    stDevMultiplier += lerp(0.0f, localizedAntiFlicker, smoothstep(0.05f, 0.95f, temporalContrast));

    AABB aabb = GetVarianceAABB(groupThreadIndex, center, stDevMultiplier); 

    // Clip history to AABB
    float3 clippedHistory = ClipToAABB(center, history, aabb.Min, aabb.Max);

    // Compute blend factor for history
    float blendFactor = GetBlendFactor(colorLuma, historyLuma, aabb.Min.x, aabb.Max.x);
    blendFactor = max(0.03f, blendFactor);

    // Blend history and current color
    float3 finalColor = lerp(clippedHistory, center, blendFactor);

    currentTAAOutputMap[pixelIndex].rgb = ConvertToOutputSpace(finalColor);
    currentTAAOutputMap[pixelIndex].a = 0.0f;
}

#endif