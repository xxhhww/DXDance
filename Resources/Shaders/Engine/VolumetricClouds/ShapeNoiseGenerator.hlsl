#ifndef _ShapeNoiseGenerator__
#define _ShapeNoiseGenerator__

#include "NoiseGeneratorHelper.hlsl"

struct PassData {
	uint  shapeNoiseMapIndex;
	uint3 shapeNoiseMapSize;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWTexture3D<float4> shapeNoiseMap = ResourceDescriptorHeap[PassDataCB.shapeNoiseMapIndex];

	float perlinToWorleyRatio = 0.3;

	float tex1RPerlinLow 	=  0.3;
	float tex1RPerlinHigh 	=  1.4;
	float tex1RWorleyLow 	= -0.3;
	float tex1RWorleyHigh 	=  1.3;
	float tex1GBAWorleyLow	= -0.4;
	float tex1GBAWorleyHigh =  1.0;

	float3 xyz = (float3)dispatchThreadID.xyz / (float3)PassDataCB.shapeNoiseMapSize;

	float perlinR = perlin7(xyz, 4.0);
	float worleyR = worley3(xyz, 6.0);
	float worleyG = worley3(xyz, 6.0);
	float worleyB = worley3(xyz, 12.0);
	float worleyA = worley3(xyz, 24.0);

	perlinR = setRange(perlinR, tex1RPerlinLow, tex1RPerlinHigh);
	worleyR = setRange(worleyR, tex1RWorleyLow, tex1RWorleyHigh);
	worleyG = setRange(worleyG, tex1GBAWorleyLow, tex1GBAWorleyHigh);
	worleyB = setRange(worleyB, tex1GBAWorleyLow, tex1GBAWorleyHigh);
	worleyA = setRange(worleyA, tex1GBAWorleyLow, tex1GBAWorleyHigh);

	float worleyPerlin = dilatePerlinWorley(perlinR, worleyR, perlinToWorleyRatio);

	shapeNoiseMap[dispatchThreadID.xyz] = float4(worleyPerlin, 1.0-worleyG, 1.0-worleyB, 1.0-worleyA);

	// DEBUG
	//Noise3D1[id.xyz] = float4(worleyPerlin, worleyPerlin, worleyPerlin, 1.0);
	//Noise3D1[id.xyz] = float4(1.0-worleyG, 1.0-worleyG, 1.0-worleyG, 1.0);
	//Noise3D1[id.xyz] = float4(1.0-worleyB, 1.0-worleyB, 1.0-worleyB, 1.0);
	//Noise3D1[id.xyz] = float4(1.0-worleyA, 1.0-worleyA, 1.0-worleyA, 1.0);
}

#endif