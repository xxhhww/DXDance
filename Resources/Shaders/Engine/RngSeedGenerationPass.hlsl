#ifndef _RngSeedGenerationPass__
#define _RngSeedGenerationPass__

struct PassData {
    uint rngSeedMapIndex;
    uint blueNoise3DMapSize;
    uint blueNoise3DMapDepth;
    uint currFrameNumber;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
    RWTexture2D<uint4> rngSeedMap = ResourceDescriptorHeap[PassDataCB.rngSeedMapIndex];

    uint2 coord = dispatchThreadID.xy;

    uint4 rngSeed = uint4(
        coord.x % PassDataCB.blueNoise3DMapSize,
        coord.y % PassDataCB.blueNoise3DMapSize,
        PassDataCB.currFrameNumber % PassDataCB.blueNoise3DMapDepth,
        false);

    rngSeedMap[coord] = rngSeed;
}

#endif