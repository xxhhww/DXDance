#ifndef _TAAPass__
#define _TAAPass__

#include "TAAHelper.hlsl"

struct PassData {
    uint2 dispatchGroupCount;
    uint  previousTAAOutputMapIndex;    // 上一帧TAA的输出结果
    uint  previousPassOutputMapIndex;   // 前一个Pass的输出结果
    uint  screenVelocityMapIndex;       // ScreenVelocity
    uint  depthStencilMapIndex;         // 深度图
    uint  currentTAAOutputMapIndex;     // 当前帧TAA的输出结果
    uint  isFirstFrame;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

#define POST_PROCESSING_BLOCK_SIZE 16
#define TILE_BORDER 1
#define TILE_SIZE (POST_PROCESSING_BLOCK_SIZE + TILE_BORDER * 2)

#define HDR_CORRECTION

groupshared uint tileRedGreen[TILE_SIZE * TILE_SIZE];
groupshared uint tileBlueDepth[TILE_SIZE * TILE_SIZE];


static float3 tonemap(float3 x)
{
	return x / (x + 1.f); // Reinhard tonemap
}

static float3 inverseTonemap(float3 x)
{
	return x / (1.f - x);
}

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
	
	uint2  pixelIndex = dispatchThreadID.xy;
    float2 pixelUV = TexelIndexToUV(pixelIndex, FrameDataCB.FinalRTResolution);


	const int2 upperLeft = IN.groupID.xy * POST_PROCESSING_BLOCK_SIZE - TILE_BORDER;

	for (uint t = IN.groupIndex; t < TILE_SIZE * TILE_SIZE; t += POST_PROCESSING_BLOCK_SIZE * POST_PROCESSING_BLOCK_SIZE)
	{
		const uint2 pixel = upperLeft + unflatten2D(t, TILE_SIZE);
		const float depth = depthBuffer[pixel];
		const float3 color = currentFrame[pixel].rgb;
		tileRedGreen[t] = f32tof16(color.r) | (f32tof16(color.g) << 16);
		tileBlueDepth[t] = f32tof16(color.b) | (f32tof16(depth) << 16);
	}
	GroupMemoryBarrierWithGroupSync();


	float3 neighborhoodMin = 100000;
	float3 neighborhoodMax = -100000;
	float3 current;
	float bestDepth = 1;

	// Search for best velocity and compute color clamping range in 3x3 neighborhood.
	int2 bestOffset = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 offset = int2(x, y);
			const uint idx = flatten2D(IN.groupThreadID.xy + TILE_BORDER + offset, TILE_SIZE);
			const uint redGreen = tileRedGreen[idx];
			const uint blueDepth = tileBlueDepth[idx];

			const float3 neighbor = float3(f16tof32(redGreen), f16tof32(redGreen >> 16), f16tof32(blueDepth));
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);
			if (x == 0 && y == 0)
			{
				current = neighbor;
			}

			const float depth = f16tof32(blueDepth >> 16);
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestOffset = offset;
			}
		}
	}

	const float2 m = motion[IN.dispatchThreadID.xy + bestOffset].xy;
	const float2 prevUV = pixelUV + m;

	float4 prev = prevFrame.SampleLevel(linearSampler, prevUV, 0);
	prev.rgb = clamp(prev.rgb, neighborhoodMin, neighborhoodMax);

	float subpixelCorrection = frac(
		max(
			abs(m.x) * cb.dimensions.x,
			abs(m.y) * cb.dimensions.y
		)
	) * 0.5f;

	float blendfactor = saturate(lerp(0.05f, 0.8f, subpixelCorrection));
	blendfactor = isSaturated(prevUV) ? blendfactor : 1.f;

#ifdef HDR_CORRECTION
	prev.rgb = tonemap(prev.rgb);
	current.rgb = tonemap(current.rgb);
#endif

	float3 resolved = lerp(prev.rgb, current.rgb, blendfactor);

#ifdef HDR_CORRECTION
	resolved.rgb = inverseTonemap(resolved.rgb);
#endif

	output[pixelIndex] = float4(resolved, 1.f);
}