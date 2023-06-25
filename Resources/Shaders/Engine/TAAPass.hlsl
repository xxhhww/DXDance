#ifndef _TAAPass__
#define _TAAPass__

struct PassData {
    uint2 dispatchGroupCount;
    uint  previousTAAOutputMapIndex;    // 上一帧TAA的输出结果
    uint  previousPassOutputMapIndex;   // 前一个Pass的输出结果
    uint  motionMapIndex;
    uint  outputMapIndex;
    float pad1;
    float pad2;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"
#include "Base/Utils.hlsl"

[numthreads(16, 16, 16)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID) {
    return;

    RWTexture2D<float4> previousTAAOutputMap  = Textures2D[PassDataCB.previousTAAOutputMapIndex];
    Texture2D           previousPassOutputMap = Textures2D[PassDataCB.previousPassOutputMapIndex];
    Texture2D<uint4>    motionMap             = Textures2D[PassDataCB.motionMapIndex];
    RWTexture2D<float4> outputMap             = Textures2D[PassDataCB.outputMapIndex];

    uint2 pixelIndex = dispatchThreadID.xy;
    uint2 pixelUV = TexelIndexToUV();

}

#endif