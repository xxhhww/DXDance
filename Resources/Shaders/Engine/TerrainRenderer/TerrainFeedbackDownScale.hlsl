#ifndef _TerrainFeedbackDownScale__
#define _TerrainFeedbackDownScale__

#include "TerrainHeader.hlsl"

struct PassData {
	uint terrainFeedbackMapIndex;
	uint terrainFeedBackBufferIndex;
	uint compareRange;
	float pad1;
}

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<uint4> terrainFeedbackMap                 = ResourceDescriptorHeap[PassDataCB.terrainFeedbackMapIndex];
	AppendStructuredBuffer<uint4> terrainFeedbackBuffer = ResourceDescriptorHeap[PassDataCB.terrainFeedBackBufferIndex];

	// 读取当前线程对应的四个像素值，并将非重复的像素值写入Buffer中
	uint2 pixelIndex = dispatchThreadID.xy;
	uint4 value0 = terrainFeedbackMap.mips[0][pixelIndex * 2];
	uint4 value1 = terrainFeedbackMap.mips[0][pixelIndex * 2 + uint2(1, 0)];
	uint4 value2 = terrainFeedbackMap.mips[0][pixelIndex * 2 + uint2(0, 1)];
	uint4 value3 = terrainFeedbackMap.mips[0][pixelIndex * 2 + uint2(1, 1)];

	terrainFeedbackBuffer.Append(value0);
	terrainFeedbackBuffer.Append(value1);
	terrainFeedbackBuffer.Append(value2);
	terrainFeedbackBuffer.Append(value3);
}

#endif