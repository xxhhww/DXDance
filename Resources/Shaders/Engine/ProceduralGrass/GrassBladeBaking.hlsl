#ifndef _GrassBladeBaking__
#define _GrassBladeBaking__

#include "GrassHelper.hlsl"

struct PassData {
	float4 terrainTileRect;		// 地块矩形，前两个分量是地形块左下角的原点，后两个分量是地块的长度

    float2 terrainWorldMeterSize;
	uint   terrainHeightMapIndex;
	float  heightScale;

	uint   clumpMapIndex;
	float  clumpMapScale;
	uint   clumpParameterBufferIndex;
	uint   clumpParameterNums;

    uint   grassBladeBufferIndex;
	uint   grassResolution;
    float  centerColorSmoothStepLower;
    float  centerColorSmoothStepUpper;

    float  jitterStrength;
    uint   grassLayerMapIndex;
    float  pad1;
    float  pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	if(dispatchThreadID.x < PassDataCB.grassResolution && dispatchThreadID.y < PassDataCB.grassResolution) {
		Texture2D<float>  terrainHeightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];
		Texture2D<float4> clumpMap = ResourceDescriptorHeap[PassDataCB.clumpMapIndex];
		StructuredBuffer<ClumpParameter> clumpParametersBuffer = ResourceDescriptorHeap[PassDataCB.clumpParameterBufferIndex];
        Texture2D<float> grassLayerMap = ResourceDescriptorHeap[PassDataCB.grassLayerMapIndex];
        AppendStructuredBuffer<BakedGrassBlade> grassBladeBuffer = ResourceDescriptorHeap[PassDataCB.grassBladeBufferIndex];

        // 草的间隔
        float grassSpacing = PassDataCB.terrainTileRect.z / PassDataCB.grassResolution;

        // Calculate xz position(假设从左下角开始撒点)
        float3 position = float3(dispatchThreadID.x, 0, dispatchThreadID.y) * grassSpacing;
        position.x += PassDataCB.terrainTileRect.x;
        position.z += PassDataCB.terrainTileRect.y;

        float2 heightUV = float2(
            (position.x + (PassDataCB.terrainWorldMeterSize.x / 2.0f)) / PassDataCB.terrainWorldMeterSize.x, 
            (position.z + (PassDataCB.terrainWorldMeterSize.x / 2.0f)) / PassDataCB.terrainWorldMeterSize.x);
        heightUV.y = 1.0f - heightUV.y;

        float  hasGrass = grassLayerMap.SampleLevel(SamplerLinearClamp, heightUV, 0u).r;
        if(hasGrass <= 0.0f) {
            return;
        }

        float  height = terrainHeightMap.SampleLevel(SamplerLinearClamp, heightUV, 0u).r * PassDataCB.heightScale;
        position.y = height;
        
        float2 hash = hashwithoutsine22(dispatchThreadID.xy);

        // Jitter xz
        float2 jitter = ((hash * 2) - 1) * PassDataCB.jitterStrength;

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
        BakedGrassBlade blade;

        blade.position = position;

        //Compute the facing by lerping between random facing and shared clump facing based on the pointInSameDirection parameter
        float2 clumpHash = hashwithoutsine22(clumpCentre);
        float2 sharedClumpFacing = normalize(tan((clumpHash + float2(0.13, 1.111)) * 2 - 1));
        float2 bladeFacing = normalize(hashwithoutsine22(dispatchThreadID.xy) * 2 - 1); 
        float2 combinedFacing = normalize(lerp(bladeFacing, sharedClumpFacing, bladeParameters.pointInSameDirection));

        blade.facing = combinedFacing;
        blade.hash = rand(dispatchThreadID.xyx);
        blade.height = baseHeight + remap01_neg11(rand(dispatchThreadID.xxy)) * heightRandom;
        blade.width = baseWidth + remap01_neg11(rand(dispatchThreadID.yxx)) * widthRandom;
        //0-1 value, controlling the vertical component of the p3 point in the bezier curve, horizontal can be derived from pythag.
        blade.tilt = baseTilt + remap01_neg11(rand(dispatchThreadID.xyx * float3(1.12, 3.3, 17.6))) * tiltRandom;
        blade.bend = baseBend + remap01_neg11(rand(dispatchThreadID.xyy * float3(12.32, 0.23, 3.39))) * bendRandom;

        /*
        float viewAlignment = abs(dot(bladeFacing, normalize(posToCam.xz)));
        float sideCurve = smoothstep(0.3, 0, viewAlignment)*1.5;     
        */
        blade.sideCurve = 0.3f * 1.5f;
        blade.type = 0u;
        /*
        float distanceToCentre = distance(blade.position.xz,  clumpCentre);
        float atten = 1 - smoothstep(PassDataCB.centerColorSmoothStepLower, PassDataCB.centerColorSmoothStepUpper, distanceToCentre);
        blade.clumpCenterDistanceFade = atten;
        */

        grassBladeBuffer.Append(blade);
	}


}



#endif