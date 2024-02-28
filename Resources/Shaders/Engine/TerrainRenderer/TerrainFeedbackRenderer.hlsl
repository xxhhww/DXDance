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

	float4 rvtRealRect;				// +x,-z ����
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

	// ���㵱ǰPatch�Ķ����ڵ���ͼ���ϵ�ƫ�����������в���

	// ��һ��������Patch����Node��Ӧ�ĵ���������ͼ���е�����
	uint tilePosX = currNodeDescriptor.tilePosX;
	uint tilePosY = currNodeDescriptor.tilePosY;
	uint tileXIndexInAtlas = tilePosX * PassDataCB.terrainAtlasTileWidthInPixels;
	uint tileYIndexInAtlas = tilePosY * PassDataCB.terrainAtlasTileWidthInPixels;

	// �ڶ��������㵱ǰPatch��Node��Ӧ�ĵ��������е�����ƫ��
	uint patchXIndexInTile = renderPatch.patchOffset.x * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.x);
	uint patchYIndexInTile = renderPatch.patchOffset.y * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.y);

	// �����������㵱ǰ������Patch�е�����ƫ��
	uint vertexXIndexInPatch = (uint)input.color.x;
	uint vertexYIndexInPatch = (uint)input.color.y;

	uint2 currIndex;
	currIndex.x = tileXIndexInAtlas + patchXIndexInTile + vertexXIndexInPatch;
	currIndex.y = tileYIndexInAtlas + patchYIndexInTile + vertexYIndexInPatch;

	v2p output;

	// �Ե��θ߶�����(heightMap albedoMap normalMap)���в���
	output.terrainHeight = terrainHeightMapAtlas.mips[0][currIndex.xy].r;
	
	float scale = pow(2, nodeLoc.z);
	input.lsPos.xz *= scale;
	input.lsPos.xz += renderPatch.position;
	input.lsPos.y = output.terrainHeight * PassDataCB.terrainHeightScale;

	// ����λ�ò���仯�����currWsPos��prevWsPos��һ����
	float3 currWsPos = input.lsPos;
	float3 prevWsPos = input.lsPos;
	float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// ǰһ֡��CsPos������Ҫ������һ֡�Ķ�������PS�м���ʱ�ټ�����һ֡��uv�������Ӷ���֤����motionVectorʱ��������
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

	// �Ƿ�Խ��
	uint overBound = (uvVT.x > 1.0f || uvVT.y > 1.0f || uvVT.x < 0.0f || uvVT.y < 0.0f) ? 1u : 0u;	

	p2o output;
	output.terrainFeedback  = uint4(pagePos, mip , overBound);

	return output;
}

#endif