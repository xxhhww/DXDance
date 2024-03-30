#ifndef _GrassBladeBaker__
#define _GrassBladeBaker__

#include "TerrainHeader.hlsl"

struct PassData {
	float2 terrainWorldMeterSize;
	float  heightScale;
	uint   terrainHeightMapAtlasIndex;

	uint   terrainNodeDescriptorListIndex;
	uint   terrainLodDescriptorListIndex;
	uint   grasslandNodeRequestTaskListIndex;
	uint   grasslandMapIndex;

	uint   grasslandNodeDescriptorListIndex;
	uint   grasslandLinearBufferIndex;
	uint   grassResolution;
	float  jitterStrength;

	uint   clumpMapIndex;
	float  clumpMapScale;
	uint   clumpParameterBufferIndex;
	uint   clumpParameterNums;

	float  grasslandNodeMeterSize;
	uint   terrainAtlasTileCountPerAxis;
	uint   terrainAtlasTileWidthInPixels;
	float  meterSpacingBetweenTerrainAtlasTilePixelsInLod0;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {

	StructuredBuffer<TerrainNodeDescriptor> terrainNodeDescriptorList = ResourceDescriptorHeap[PassDataCB.terrainNodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  terrainLodDescriptorList  = ResourceDescriptorHeap[PassDataCB.terrainLodDescriptorListIndex];
	Texture2D                               terrainHeightMapAtlas     = ResourceDescriptorHeap[PassDataCB.terrainHeightMapAtlasIndex];

	Texture2D<float4>                clumpMap              = ResourceDescriptorHeap[PassDataCB.clumpMapIndex];
	StructuredBuffer<ClumpParameter> clumpParametersBuffer = ResourceDescriptorHeap[PassDataCB.clumpParameterBufferIndex];

	Texture2D<float>                              grasslandMap                 = ResourceDescriptorHeap[PassDataCB.grasslandMapIndex];
	StructuredBuffer<GpuGrasslandNodeRequestTask> grasslandNodeRequestTaskList = ResourceDescriptorHeap[PassDataCB.grasslandNodeRequestTaskListIndex];
	RWStructuredBuffer<GrasslandNodeDescriptor>   grasslandNodeDescriptorList  = ResourceDescriptorHeap[PassDataCB.grasslandNodeDescriptorListIndex];
	RWStructuredBuffer<GrassBladeDescriptor>      grasslandLinearBuffer        = ResourceDescriptorHeap[PassDataCB.grasslandLinearBufferIndex];

	// 获取该线程盒对应的RequestTask和对应的GrasslandNodeDescriptor
    GpuGrasslandNodeRequestTask grasslandNodeRequestTask = grasslandNodeRequestTaskList[dispatchThreadID.x];
    GrasslandNodeDescriptor     grasslandNodeDescriptor  = grasslandNodeDescriptorList[grasslandNodeRequestTask.nextGrasslandNodeIndex];

    // 计算写入索引(tileIndex描述的是LinearBuffer中的第几个TileBuffer，而一个TileBuffer记录了一个GrasslandNode中所有的草点)
    uint writeIndex = grasslandNodeRequestTask.tileIndex * (PassDataCB.grassResolution * PassDataCB.grassResolution) + (dispatchThreadID.y * PassDataCB.grassResolution + dispatchThreadID.z);

	// 计算草的间隔
    float grassSpacing = PassDataCB.grasslandNodeMeterSize / (float)PassDataCB.grassResolution;

	// 计算GrassBladePosition
	// 计算WsPositionXZ(从左上角做偏移)
	uint grasslandNodeLocationX = grasslandNodeDescriptor.nodeLocationX;
	uint grasslandNodeLocationY = grasslandNodeDescriptor.nodeLocationY;
	uint grasslandNodeMeterSize = grasslandNodeDescriptor.nodeMeterSize;

	float3 wsPosition = float3(0.0f, 0.0f, 0.0f);
	wsPosition.x = grasslandNodeLocationX * grasslandNodeMeterSize;
	wsPosition.z = grasslandNodeLocationY * grasslandNodeMeterSize;
	wsPosition.x = wsPosition.x - PassDataCB.terrainWorldMeterSize.x / 2.0f;
	wsPosition.z = PassDataCB.terrainWorldMeterSize.x / 2.0f - wsPosition.z;

	// 计算PositionY
	// 定位到当前草点所属的TerrainNodeDescriptor
	TerrainLodDescriptor terrainLod0Descriptor = terrainLodDescriptorList[0];
	uint terrainNodeDescriptorIndex = terrainLod0Descriptor.nodeStartOffset + grasslandNodeRequestTask.nextGrasslandNodeIndex;
	TerrainNodeDescriptor terrainNodeDescriptor = terrainNodeDescriptorList[terrainNodeDescriptorIndex];

	uint2 terrainAtlasTilePos = uint2(terrainNodeDescriptor.tilePosX, terrainNodeDescriptor.tilePosY);
	uint2 pixelIndexInTerrainAtlas = terrainAtlasTilePos * PassDataCB.terrainAtlasTileWidthInPixels;

	// GrassBladeBaker是在一个64m * 64m的区域内进行撒点，通过dispatchThreadID.yz可以计算当前的GrassBlade在该区域内的偏移
	float2 localOffsetInCurrNode = float2(dispatchThreadID.z * grassSpacing, dispatchThreadID.y * grassSpacing);

	// 计算blade附近的四个整数位置点
	uint2 leftTopIndex     = uint2(floor(localOffsetInCurrNode.x),                                                              floor(localOffsetInCurrNode.y));
	uint2 rightTopIndex    = uint2(floor(localOffsetInCurrNode.x + PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0), floor(localOffsetInCurrNode.y));
	uint2 leftBottomIndex  = uint2(floor(localOffsetInCurrNode.x),                                                              floor(localOffsetInCurrNode.y + PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0));
	uint2 rightBottomIndex = uint2(floor(localOffsetInCurrNode.x + PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0), floor(localOffsetInCurrNode.y + PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0));

	// 取出四个整数位置上的值
	float leftTopValue     = terrainHeightMapAtlas.mips[0][pixelIndexInTerrainAtlas + leftTopIndex].r;
	float rightTopValue    = terrainHeightMapAtlas.mips[0][pixelIndexInTerrainAtlas + rightTopIndex].r;
	float leftBottomValue  = terrainHeightMapAtlas.mips[0][pixelIndexInTerrainAtlas + leftBottomIndex].r;
	float rightBottomValue = terrainHeightMapAtlas.mips[0][pixelIndexInTerrainAtlas + rightBottomIndex].r;

	// 计算双线性插值
	/*
	* LT-----Top-----RT
	* |               |
	* |     Blade     |
	* |               |
	* LB-----Bot-----RB
	*/

	float lerpFactorX = 1.0f - ((localOffsetInCurrNode.x - leftTopIndex.x) / PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0);
	float lerpFactorY = 1.0f - ((localOffsetInCurrNode.y - leftTopIndex.y) / PassDataCB.meterSpacingBetweenTerrainAtlasTilePixelsInLod0);

	float topValue    = leftTopValue    * lerpFactorX + rightTopValue    * (1.0f - lerpFactorX);
	float botValue    = leftBottomValue * lerpFactorY + rightBottomValue * (1.0f - lerpFactorX);
	float heightValue = topValue * lerpFactorY + botValue * (1.0f - lerpFactorY);

	wsPosition.y = heightValue * PassDataCB.heightScale;


	float2 hash = hashwithoutsine22(dispatchThreadID.zy);
    // Jitter xz
    float2 jitter = ((hash * 2.0f) - 1.0f) * PassDataCB.jitterStrength;
    wsPosition.xz += jitter;

    float2 clumpUV = wsPosition.xz * float2(PassDataCB.clumpMapScale.xx);

    // Retrieve clump data for this blade from voronoi texture
    // This includes the clump parameter id, and the centre of this clump in texture space
    float3 clumpData = clumpMap.SampleLevel(SamplerLinearWrap, clumpUV, 0u).rgb;

    // This is the index of the clump parameter set for this blade
    float clumpParamsIndex = clumpData.x;

    // Retrieve the correct set of blade parameters
    ClumpParameter bladeParameters = clumpParametersBuffer[int((clumpParamsIndex))]; 

    // Compute the clump centre in world space by finding its offsetted position in texture space and -dividing- that by the voronoi tiling 
    float2 clumpCentre = (clumpData.yz + floor(clumpUV)) / float2(PassDataCB.clumpMapScale.xx);

    // Pull position to centre of clump based on pullToCentre
    wsPosition.xz = lerp(wsPosition.xz, clumpCentre, bladeParameters.pullToCentre);

    // Copy parameters from parameter struct
    // float4 clumpColor = bladeParameters.clumpColor;
    float baseHeight   = bladeParameters.baseHeight;
    float heightRandom = bladeParameters.heightRandom;
    float baseWidth    = bladeParameters.baseWidth;
    float widthRandom  = bladeParameters.widthRandom;
    float baseTilt     = bladeParameters.baseTilt;
    float tiltRandom   = bladeParameters.tiltRandom;
    float baseBend     = bladeParameters.baseBend;
    float bendRandom   = bladeParameters.bendRandom;

	// Start building grassblade struct
    GrassBladeDescriptor blade;

    blade.position = wsPosition;

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
        
    grasslandLinearBuffer[writeIndex] = blade;
}

#endif