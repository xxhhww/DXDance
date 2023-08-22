#ifndef _TAAPass__
#define _TAAPass__

#include "TAAHelper.hlsl"

struct PassData {
    uint2 dispatchGroupCount;
    uint  previousTAAOutputMapIndex;    // 上一帧TAA的输出结果
    uint  previousPassOutputMapIndex;   // 前一个Pass的输出结果
    uint  gBufferMotionVectorMapIndex;  // GBufferMotionVector
    uint  depthStencilMapIndex;         // 深度图
    uint  currentTAAOutputMapIndex;     // 当前帧TAA的输出结果
    uint  isFirstFrame;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Base/ThreadGroupTilingX.hlsl"
#include "../Math/MathCommon.hlsl"

groupshared min16float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

void LoadNeighbors(uint2 pixelIndex, int2 GTid, Texture2D image)
{
    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(pixelIndex, GTid, FrameDataCB.FinalRTResolution, GroupDimensionSize, SamplingRadius);

    float3 centerColor = ConvertToWorkingSpace(image[coords.LoadCoord0].rgb);

    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = min16float3(centerColor);

    if (coords.IsLoadStore1Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord1].rgb);
        gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = min16float3(color);
    }

    if (coords.IsLoadStore2Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord2].rgb);
        gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = min16float3(color);
    }

    if (coords.IsLoadStore3Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord3].rgb);
        gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = min16float3(color);
    }

    GroupMemoryBarrierWithGroupSync();
}

float3 SampleHistory(Texture2D historyTex, float2 uv)
{
    float3 history = historyTex.SampleLevel(SamplerLinearClamp, uv, 0.0).rgb;
    history = ConvertToWorkingSpace(history);
    return history;
}

AABB GetVarianceAABB(int2 GTindex, float3 center, float stDevMultiplier)
{
    float3 M1 = center; // First moment - Average
    float3 M2 = Square(center); // Second moment - Variance
    float sampleCount = 1.0;

    [unroll] 
    for (int x = -SamplingRadius; x <= SamplingRadius; ++x)
    {
        [unroll]
        for (int y = -SamplingRadius; y <= SamplingRadius; ++y)
        {
            if (x == 0 && y == 0)
                continue;

            int2 loadCoord = GTindex + int2(x, y);
            float3 color = gCache[loadCoord.x][loadCoord.y];

            M1 += color;
            M2 += Square(color);
            sampleCount += 1.0;
        }
    }

    float3 MU = M1 / sampleCount;
    float3 sigma = sqrt(max(M2 / sampleCount - MU * MU, 0.0));

    return InitAABB(MU - stDevMultiplier * sigma, MU + stDevMultiplier * sigma);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 GTid : SV_GroupThreadID, int3 Gid : SV_GroupID)
{
    Texture2D<float4>   previousFrameTexture = ResourceDescriptorHeap[PassDataCB.previousTAAOutputMapIndex];
    Texture2D<float4>   currentFrameTexture = ResourceDescriptorHeap[PassDataCB.previousPassOutputMapIndex];
    Texture2D<float4>   motionTexture = ResourceDescriptorHeap[PassDataCB.gBufferMotionVectorMapIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[PassDataCB.currentTAAOutputMapIndex];

    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.dispatchGroupCount, GroupDimensionSize.xx, 8, GTid.xy, Gid.xy);
    
    float3 motion = motionTexture[pixelIndex].xyz;
    float2 uv = TexelIndexToUV(pixelIndex, FrameDataCB.FinalRTResolution);
    float2 reprojectedUV = uv - motion.xy;


    {
        outputTexture[pixelIndex].rgba = currentFrameTexture[pixelIndex].rgba;
        return;
    }

    // 当前帧为第一帧
    if (PassDataCB.isFirstFrame) {
        outputTexture[pixelIndex].rgba = currentFrameTexture[pixelIndex].rgba;
        return;
    }

    LoadNeighbors(pixelIndex, GTid.xy, currentFrameTexture);

    // Variance clipping
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    // https://en.wikipedia.org/wiki/Moment_(mathematics)

    int2 groupThreadIndex = GTid.xy + SamplingRadius;
    float3 center = gCache[groupThreadIndex.x][groupThreadIndex.y];
    float3 history = SampleHistory(previousFrameTexture, reprojectedUV);
    float colorLuma = GetLuminance(center);
    float historyLuma = GetLuminance(history);

    float stDevMultiplier = 1.0;

    // The reasoning behind the anti flicker is that if we have high spatial contrast (high standard deviation)
    // and high temporal contrast, we let the history to be closer to be unclipped. To achieve, the min/max bounds
    // are extended artificially more.
    float temporalContrast = saturate(abs(colorLuma - historyLuma) / max(0.2, max(colorLuma, historyLuma)));
     
    float motionVectorLen = length(motion.xy);
    float screenDiag = length(float2(FrameDataCB.FinalRTResolution));
    const float MaxFactorScale = 2.25f; // when stationary
    const float MinFactorScale = 0.8f; // when moving more than slightly
    // Anti-flicker factor is shrunk under motion
    float localizedAntiFlicker = lerp(MinFactorScale, MaxFactorScale, saturate(1.0f - 2.0f * (motionVectorLen * screenDiag)));

    // Extend AABB if temporal contrast is high
    stDevMultiplier += lerp(0.0, localizedAntiFlicker, smoothstep(0.05, 0.95, temporalContrast));

    AABB aabb = GetVarianceAABB(groupThreadIndex, center, stDevMultiplier); 

    // Clip history to AABB
    float3 clippedHistory = ClipToAABB(center, history, aabb.Min, aabb.Max);

    // Compute blend factor for history
    float blendFactor = GetBlendFactor(colorLuma, historyLuma, aabb.Min.x, aabb.Max.x);
    blendFactor = max(0.03f, blendFactor);

    // Blend history and current color
    float3 finalColor = lerp(clippedHistory, center, blendFactor);

    outputTexture[pixelIndex].rgb = ConvertToOutputSpace(finalColor);
}

#endif