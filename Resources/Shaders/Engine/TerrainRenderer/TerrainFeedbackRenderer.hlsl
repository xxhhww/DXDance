#ifndef _TerrainFeedbackRenderer__
#define _TerrainFeedbackRenderer__

#include "TerrainHeader.hlsl"

struct PassData {
	float2 terrainMeterSize;
	float  terrainHeightScale;
	uint   culledPatchListIndex;

	uint  nodeDescriptorListIndex;
	uint  lodDescriptorListIndex;
	uint  terrainHeightMapAtlasIndex;
	uint  terrainAtlasTileCountPerAxis;

	uint  terrainAtlasTileWidthInPixels;
	uint  terrainPatchVertexCountPerAxis;
	uint  tileCountPerAxisInPage0Level;
	uint  scaledVirtualTextureSizeInBytesInPage0Level;

	uint  maxPageLevel;
	uint  pageLevelBias;
	float pad1;
	float pad2;

	float4 rvtRealRect;				// +x,-z 方向
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
};

struct p2o {
	uint4  terrainFeedback : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch>           culledPatchList    = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];
	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	Texture2D terrainHeightMapAtlas = ResourceDescriptorHeap[PassDataCB.terrainHeightMapAtlasIndex];

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

	// 对地形高度纹理(heightMap albedoMap normalMap)进行采样
	output.terrainHeight = terrainHeightMapAtlas.mips[0][currIndex.xy].r;
	
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
	output.uv = input.uv;
	output.nodeLod = nodeLoc.z;

	return output;
}

p2o PSMain(v2p input) {
	float2 uvVT = float2(0.0f, 0.0f);
	uvVT.x = (input.wsPos.x - PassDataCB.rvtRealRect.x) / PassDataCB.rvtRealRect.z;
	uvVT.y = (PassDataCB.rvtRealRect.y - input.wsPos.z) / PassDataCB.rvtRealRect.w;

	// Calcute Feedback
	uint2 pagePos = floor(uvVT * (float)PassDataCB.tileCountPerAxisInPage0Level);
	
	float2 uv = uvVT * (float)PassDataCB.scaledVirtualTextureSizeInBytesInPage0Level;
	float2 dx = ddx(uv);
	float2 dy = ddy(uv);
	int mip = clamp(int(0.5 * log2(max(dot(dx, dx), dot(dy, dy))) + 0.5 + PassDataCB.pageLevelBias), 0, PassDataCB.maxPageLevel);

	// 是否越界
	uint overBound = (uvVT.x > 1.0f || uvVT.y > 1.0f || uvVT.x < 0.0f || uvVT.y < 0.0f) ? 1u : 0u;	

	p2o output;
	output.terrainFeedback  = uint4(pagePos, mip , overBound);

	return output;
}

#endif