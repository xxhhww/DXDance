#ifndef _TerrainQuadTreeFarRenderer__
#define _TerrainQuadTreeFarRenderer__

#include "TerrainHeader.hlsl"

struct PassData {
	float2 terrainMeterSize;
	float  terrainHeightScale;
	uint   culledPatchListIndex;

	uint  nearCulledPatchListIndex;
	uint  farCulledPatchListIndex;
	uint  nodeDescriptorListIndex;
	uint  lodDescriptorListIndex;

	uint  terrainHeightMapAtlasIndex;
	uint  terrainAlbedoMapAtlasIndex;
	uint  terrainNormalMapAtlasIndex;
	uint  lodDebug;

	uint  terrainAtlasTileCountPerAxis;
	uint  terrainAtlasTileWidthInPixels;
	uint  terrainPatchVertexCountPerAxis;
	float pad1;

	// x: page table size
	// y: virtual texture size
	// z: max mipmap level
	// w: mipmap level bias
	float4 vtFeedbackParams;
	float4 vtRealRect;
	// x: padding size
	// y: tileSize
	// z: physical texture size x
	// w: physical texture size y
	float4 vtPhysicalMapParams;

	uint  runtimeVTPageTableMapIndex;
	uint  runtimeVTAlbedoAtlasIndex;
	uint  runtimeVTNormalAtlasIndex;
	float pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float4 color     : COLOR;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
	float2 uv        : TEXCOORD2;
	uint   nodeLod   : NODELOD;

	float  terrainHeight : TerrainHeight;
	float3 terrainAlbedo : TerrainAlbedo;
	float3 terrainNormal : TerrainNormal;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3; 
	float  viewDepth        : SV_TARGET4;
};

// 根据Node的索引获得Node的全局ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.terrainMeterSize.x / currLodDescriptor.nodeMeterSize;

    return currLodDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch>           culledPatchList    = ResourceDescriptorHeap[PassDataCB.farCulledPatchListIndex];
	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	Texture2D terrainHeightMapAtlas = ResourceDescriptorHeap[PassDataCB.terrainHeightMapAtlasIndex];
	Texture2D terrainAlbedoMapAtlas = ResourceDescriptorHeap[PassDataCB.terrainAlbedoMapAtlasIndex];
	Texture2D terrainNormalMapAtlas = ResourceDescriptorHeap[PassDataCB.terrainNormalMapAtlasIndex];

	RenderPatch renderPatch = culledPatchList[instanceID];
	uint3 nodeLoc = renderPatch.nodeLoc;

	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[nodeLoc.z];
	uint nodeCountPerRow = PassDataCB.terrainMeterSize.x / currLodDescriptor.nodeMeterSize;
    uint globalNodeID = currLodDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
	TerrainNodeDescriptor currNodeDescriptor = nodeDescriptorList[globalNodeID];

	// 计算当前Patch的顶点在地形图集上的偏移量，并进行采样

	// 第一步，计算Patch所属Node对应的地形纹理在图集中的索引
	uint tilePosX = currNodeDescriptor.tilePosX;
	uint tilePosY = currNodeDescriptor.tilePosY;
	uint tileXIndexInAtlas = tilePosX * PassDataCB.terrainAtlasTileWidthInPixels;
	uint tileYIndexInAtlas = tilePosY * PassDataCB.terrainAtlasTileWidthInPixels;

	// 第二步，计算当前Patch在Node对应的地形纹理中的索引偏移
	uint patchXIndexInTile = renderPatch.patchOffset.x * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.x);
	uint patchYIndexInTile = renderPatch.patchOffset.y * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.y);

	// 第三步，计算当前顶点在Patch中的索引偏移
	uint vertexXIndexInPatch = (uint)input.color.x;
	uint vertexYIndexInPatch = (uint)input.color.y;

	uint2 currIndex;
	currIndex.x = tileXIndexInAtlas + patchXIndexInTile + vertexXIndexInPatch;
	currIndex.y = tileYIndexInAtlas + patchYIndexInTile + vertexYIndexInPatch;

	v2p output;

	// 对地形纹理图形(heightMap albedoMap normalMap)进行采样
	output.terrainHeight = terrainHeightMapAtlas.mips[0][currIndex.xy].r;
	output.terrainAlbedo = terrainAlbedoMapAtlas.mips[0][currIndex.xy].rgb;
	output.terrainNormal = terrainNormalMapAtlas.mips[0][currIndex.xy].rgb;

	output.terrainNormal.xz = output.terrainNormal.xy * 2.0f - 1.0f;
    output.terrainNormal.y  = sqrt(max(0u, 1u - dot(output.terrainNormal.xz, output.terrainNormal.xz)));
	output.terrainNormal = normalize(output.terrainNormal);

	float scale = pow(2, nodeLoc.z);
	input.lsPos.xz *= scale;
	input.lsPos.xz += renderPatch.position;
	input.lsPos.y = output.terrainHeight * PassDataCB.terrainHeightScale;

	// 地形位置不会变化，因此currWsPos与prevWsPos是一样的
	float3 currWsPos = input.lsPos;
	float3 prevWsPos = input.lsPos;

	float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);


	output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;
	// output.uvHeight = heightUV;
	// output.uvVT = (currWsPos.xz - PassDataCB.vtRealRect.xy) / PassDataCB.vtRealRect.zw;
	output.uv = input.uv;
	output.nodeLod = nodeLoc.z;
	return output;	
}

p2o PSMain(v2p input) {
	Texture2D runtimeVTAlbedoAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTAlbedoAtlasIndex];
	Texture2D runtimeVTNormalAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTNormalAtlasIndex];
	Texture2D runtimeVTPageTableMap = ResourceDescriptorHeap[PassDataCB.runtimeVTPageTableMapIndex];

	float3 currLodColor = GetLODColor(input.nodeLod);
	float3 runtimeVTAlbedo = runtimeVTAlbedoAtlas.SampleLevel(SamplerLinearWrap, input.uv, 0u).rgb;
	float3 runtimeVTNormal = runtimeVTNormalAtlas.SampleLevel(SamplerLinearWrap, input.uv, 0u).rgb;
	uint3  runtimeVTPageTable = runtimeVTPageTableMap.SampleLevel(SamplerPointWrap, input.uv, 0u).rgb;
	currLodColor = currLodColor + runtimeVTNormal * 0.01f + runtimeVTPageTable * 0.001f;

	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	float3 albedo = currLodColor;
	float3 normal = float3(0.0f, 1.0f, 0.0f);

	p2o output;
	output.albedoMetalness  = float4(currLodColor, 0.0f);
	output.positionEmission = float4(input.wsPos, 0.0f);
	output.normalRoughness  = float4(input.terrainNormal.rgb, 1.0f);
	output.motionVector     = float4(velocity.xy, 0.0f, 0.0f);
	output.viewDepth        = input.vsPos.z;

	return output;
}

#endif