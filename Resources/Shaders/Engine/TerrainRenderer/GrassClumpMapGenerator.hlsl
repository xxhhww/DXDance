#ifndef _GrassClumpMapGenerator__
#define _GrassClumpMapGenerator__

struct PassData{
	float2 clumpMapSize;
	uint   numClumps;
	uint   clumpMapIndex;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    RWTexture2D<float4> clumpMap = ResourceDescriptorHeap[PassDataCB.clumpMapIndex];

	uint2 pixelIndex = dispatchThreadID.xy;

	float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.clumpMapSize);

    float minDist = 100000;
    float id = 12;

    float2 clumpCentre = float2(0, 0);

    for (int j = 1; j < 40; j++) {
        float2 jj = float2(j,j);
        float2 p =  N22(jj);

        float  d = distance(p, uv);
        if (d < minDist) {    
            minDist = d;
            id = fmod((int)j, (int)PassDataCB.numClumps);
            clumpCentre = p;
        }
    }
    
    clumpMap[pixelIndex] = float4(float3(id, clumpCentre), 1.0f);
}

#endif