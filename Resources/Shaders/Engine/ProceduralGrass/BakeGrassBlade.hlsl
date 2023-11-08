#ifndef _BakeGrassBlade__
#define _BakeGrassBlade__

#include "ProceduralGrassHelper.hlsl"

struct PassData {
    float2 terrainWorldMeterSize;
	uint   terrainHeightMapIndex;
	float  heightScale;

	uint   clumpMapIndex;
	float  clumpMapScale;
	uint   clumpParameterBufferIndex;
	uint   clumpParameterNums;

	uint   needBakedGrassClusterListIndex;
	uint   grassLayerMaskIndex;
    uint   bakedGrassBladeListIndex;
	uint   grassResolution;

    float  jitterStrength;
    float  pad1;
    float  pad2;
	float  pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	Texture2D<float>                 terrainHeightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];
	Texture2D<float4>                clumpMap = ResourceDescriptorHeap[PassDataCB.clumpMapIndex];
	StructuredBuffer<ClumpParameter> clumpParametersBuffer = ResourceDescriptorHeap[PassDataCB.clumpParameterBufferIndex];
	StructuredBuffer<GrassCluster>   needBakedGrassClusterList = ResourceDescriptorHeap[PassDataCB.needBakedGrassClusterListIndex];
    Texture2D<float>                 grassLayerMask = ResourceDescriptorHeap[PassDataCB.grassLayerMaskIndex];
    RWStructuredBuffer<GrassBlade>   bakedGrassBladeList = ResourceDescriptorHeap[PassDataCB.bakedGrassBladeListIndex];

    // 获取该线程盒处理的GrassCluster
    GrassCluster grassCluster = needBakedGrassClusterList[dispatchThreadID.x];
    
    // 计算写入索引
    uint writeIndex = grassCluster.grassBladeBufferIndex + (dispatchThreadID.y * PassDataCB.grassResolution + dispatchThreadID.z);

	// 计算草的间隔
    float grassSpacing = (float)grassCluster.grassClusterRect.z / (float)PassDataCB.grassResolution;

    // Calculate xz position(假设从左下角开始撒点)
    float3 position = float3(dispatchThreadID.z, 0, dispatchThreadID.y) * grassSpacing;
    position.x += grassCluster.grassClusterRect.x;
    position.z += grassCluster.grassClusterRect.y;

    float2 heightUV = (position.xz + (PassDataCB.terrainWorldMeterSize * 0.5f) + 0.5f) / (PassDataCB.terrainWorldMeterSize + 1.0f);
    heightUV.y = 1.0f - heightUV.y;
    // TODO控制密度
    /*
    float  hasGrass = grassLayerMask.SampleLevel(SamplerPointClamp, heightUV, 0u).r;
    if(hasGrass <= 0.0f) {
        GrassBlade emptyGrassBlade = CreateEmptyGrassBlade();
        bakedGrassBladeList[writeIndex] = emptyGrassBlade;
        return;
    }
    */

    float  height = terrainHeightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r * PassDataCB.heightScale;
    position.y = height;
        
    float2 hash = hashwithoutsine22(dispatchThreadID.zy);

    // Jitter xz
    float2 jitter = ((hash * 2.0f) - 1.0f) * PassDataCB.jitterStrength;

    position.xz += jitter;

    float2 clumpUV = position.xz * float2(PassDataCB.clumpMapScale.xx);

    // Retrieve clump data for this blade from voronoi texture
    // This includes the clump parameter id, and the centre of this clump in texture space
    float3 clumpData = clumpMap.SampleLevel(SamplerLinearWrap, clumpUV, 0u).rgb;

    //This is the index of the clump parameter set for this blade
    float clumpParamsIndex = clumpData.x;

    //Retrieve the correct set of blade parameters
    ClumpParameter bladeParameters = clumpParametersBuffer[int((clumpParamsIndex))]; 

    //Compute the clump centre in world space by finding its offsetted position in texture space and -dividing- that by the voronoi tiling 
    float2 clumpCentre = (clumpData.yz + floor(clumpUV)) / float2(PassDataCB.clumpMapScale.xx);

    //Pull position to centre of clump based on pullToCentre
    position.xz = lerp(position.xz, clumpCentre, bladeParameters.pullToCentre);

    //Copy parameters from parameter struct
    //float4 clumpColor = bladeParameters.clumpColor;
    float baseHeight = bladeParameters.baseHeight;
    float heightRandom = bladeParameters.heightRandom;
    float baseWidth = bladeParameters.baseWidth;
    float widthRandom = bladeParameters.widthRandom;
    float baseTilt = bladeParameters.baseTilt;
    float tiltRandom = bladeParameters.tiltRandom;
    float baseBend = bladeParameters.baseBend;
    float bendRandom = bladeParameters.bendRandom;
            
    //Start building grassblade struct
    GrassBlade blade;

    blade.position = position;

    //Compute the facing by lerping between random facing and shared clump facing based on the pointInSameDirection parameter
    float2 clumpHash = hashwithoutsine22(clumpCentre);
    float2 sharedClumpFacing = normalize(tan((clumpHash + float2(0.13, 1.111)) * 2 - 1));
    float2 bladeFacing = normalize(hashwithoutsine22(dispatchThreadID.zy) * 2 - 1); 
    float2 combinedFacing = normalize(lerp(bladeFacing, sharedClumpFacing, bladeParameters.pointInSameDirection));

    blade.facing = combinedFacing;
    blade.hash = rand(dispatchThreadID.zyz);
    blade.height = baseHeight + remap01_neg11(rand(dispatchThreadID.zzy)) * heightRandom;
    blade.width = baseWidth + remap01_neg11(rand(dispatchThreadID.yzz)) * widthRandom;
    //0-1 value, controlling the vertical component of the p3 point in the bezier curve, horizontal can be derived from pythag.
    blade.tilt = baseTilt + remap01_neg11(rand(dispatchThreadID.zyz * float3(1.12, 3.3, 17.6))) * tiltRandom;
    blade.bend = baseBend + remap01_neg11(rand(dispatchThreadID.zyy * float3(12.32, 0.23, 3.39))) * bendRandom;
    blade.sideCurve = 0.3f * 1.5f;
        
    bakedGrassBladeList[writeIndex] = blade;
}

#endif