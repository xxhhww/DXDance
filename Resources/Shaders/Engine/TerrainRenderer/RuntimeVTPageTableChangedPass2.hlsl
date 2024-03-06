#ifndef _RuntimeVTPageTablePass2__
#define _RuntimeVTPageTablePass2__

#include "TerrainHeader.hlsl"

struct PassData{
	uint srcRuntimeVTPageTableMapIndex;		// source
	uint dstRuntimeVTPageTableMapIndex;		// target
	int2 pixelOffset;						// pageÆ«ÒÆ

	int   pageTableMapSize;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
	Texture2D<uint4>   srcRuntimeVTPageTableMap = ResourceDescriptorHeap[PassDataCB.srcRuntimeVTPageTableMapIndex];
	RWTexture2D<uint4> dstRuntimeVTPageTableMap = ResourceDescriptorHeap[PassDataCB.dstRuntimeVTPageTableMapIndex];

	// Àý£º
	// pixelOffset = int2(0, 32) 
	// srcPixelIndex = (64, 128) dstPixelIndex = (64, 160)
	// srcPixelIndex = (64, 234) dstPixelIndex = (64, (234 + 32) - 256)
	// pixelOffset = int2(0, -32)
	// srcPixelIndex = (64, 15)  dstPixelIndex = (64, (15 - 32) + 256)
	int2 srcPixelIndex = dispatchThreadID.xy;
	int2 dstPixelIndex = srcPixelIndex;
	dstPixelIndex.x = dstPixelIndex.x - PassDataCB.pixelOffset.x;
	dstPixelIndex.y = dstPixelIndex.y + PassDataCB.pixelOffset.y;

	if(dstPixelIndex.x < 0)                                    dstPixelIndex.x += PassDataCB.pageTableMapSize;
	else if(dstPixelIndex.x > PassDataCB.pageTableMapSize - 1) dstPixelIndex.x -= PassDataCB.pageTableMapSize;

	if(dstPixelIndex.y < 0)                                    dstPixelIndex.y += PassDataCB.pageTableMapSize;
	else if(dstPixelIndex.y > PassDataCB.pageTableMapSize - 1) dstPixelIndex.y -= PassDataCB.pageTableMapSize;

	dstRuntimeVTPageTableMap[dstPixelIndex.xy] = srcRuntimeVTPageTableMap[srcPixelIndex.xy];
}

#endif