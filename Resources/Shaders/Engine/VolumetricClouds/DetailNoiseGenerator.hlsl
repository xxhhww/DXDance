#ifndef _DetailNoiseGenerator__
#define _DetailNoiseGenerator__

#include "NoiseGeneratorHelper.hlsl"

struct PassData {
	uint  detailNoiseMapIndex;
	uint3 detailNoiseMapSize;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWTexture3D<float4> detailNoiseMap = ResourceDescriptorHeap[PassDataCB.detailNoiseMapIndex];

	float tex2Low = -0.2;
	float tex2High = 1.0;

	float3 xyz = (float3)dispatchThreadID.xyz / (float)PassDataCB.detailNoiseMapSize;

	float worleyR = worley3(xyz, 10);
	float worleyG = worley3(xyz, 15);
	float worleyB = worley3(xyz, 20);
	float worleyA = worley3(xyz, 10);

	worleyR = setRange(worleyR, tex2Low, tex2High);
	worleyG = setRange(worleyG, tex2Low, tex2High);
	worleyB = setRange(worleyB, tex2Low, tex2High);
	worleyA = setRange(worleyA, tex2Low, tex2High);

	detailNoiseMap[dispatchThreadID.xyz] = float4(worleyR, worleyG, worleyB, worleyA);
}

#endif