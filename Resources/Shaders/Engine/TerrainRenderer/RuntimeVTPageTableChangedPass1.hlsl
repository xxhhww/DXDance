#ifndef _RuntimeVTPageTablePass1__
#define _RuntimeVTPageTablePass1__

#include "TerrainHeader.hlsl"

struct PassData{
	uint  runtimeVTPageTableMapIndex;
	uint  maxPageLevel;
	uint2 invalidPageLevel;		// ����һ�ηǷ���PageLevel

	int2  invalidRegionX;		// ����һ�ηǷ�����(u��)
	int2  invalidRegionY;		// ����һ�ηǷ�����(v��)
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	RWTexture2D<uint4> runtimeVTPageTable = ResourceDescriptorHeap[PassDataCB.runtimeVTPageTableMapIndex];

	uint2 pixelIndex = dispatchThreadID.xy;

	uint4 indexData = runtimeVTPageTable[pixelIndex.xy];

	// ���ڷǷ����䣬�����÷Ƿ����(maxPageLevel + 1)
	if((pixelIndex.x >= PassDataCB.invalidRegionX.x && pixelIndex.x <= PassDataCB.invalidRegionX.y) ||
	   (pixelIndex.y >= PassDataCB.invalidRegionY.x && pixelIndex.y <= PassDataCB.invalidRegionY.y) ||
	   (indexData.z >= PassDataCB.invalidPageLevel.x && indexData.z <= PassDataCB.invalidPageLevel.y)) {
	   runtimeVTPageTable[pixelIndex.xy] = PassDataCB.maxPageLevel + 1;
	}
}

#endif