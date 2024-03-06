#ifndef _TerrainQuadTreeNearRenderer__
#define _TerrainQuadTreeNearRenderer__

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
	uint  pageLevelDebug;

	uint  runtimeVTPageTableMapIndex;
	uint  runtimeVTAlbedoAtlasIndex;
	uint  runtimeVTNormalAtlasIndex;
	float runtimeVTAtlasSize;

	float4 runtimeVTRealRect;

	float runtimeVTTileCountPerAxisInPage0Level;
	float runtimeVTMaxPageLevel;						// ������ߵ�PageLevel,������ʵ����ߵ�PageLevel
	float tilePaddingSize;
	float tileSizeNoPadding;
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

// ����Node���������Node��ȫ��ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.terrainMeterSize.x / currLodDescriptor.nodeMeterSize;

    return currLodDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<RenderPatch>           culledPatchList    = ResourceDescriptorHeap[PassDataCB.nearCulledPatchListIndex];
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

	// �Ե�������ͼ��(heightMap albedoMap normalMap)���в���
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
	// output.uvHeight = heightUV;
	// output.uvVT = (currWsPos.xz - PassDataCB.vtRealRect.xy) / PassDataCB.vtRealRect.zw;
	output.uv = input.uv;
	output.nodeLod = nodeLoc.z;
	return output;	
}

p2o PSMain(v2p input) {
	Texture2D runtimeVTAlbedoAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTAlbedoAtlasIndex];
	Texture2D runtimeVTNormalAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTNormalAtlasIndex];
	Texture2D<uint4> runtimeVTPageTableMap = ResourceDescriptorHeap[PassDataCB.runtimeVTPageTableMapIndex];

	// ��ǰ֡��uv����
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	// ����uvVT
	float2 uvVT = float2(0.0f, 0.0f);
	uvVT.x = (input.wsPos.x - PassDataCB.runtimeVTRealRect.x) / PassDataCB.runtimeVTRealRect.z;
	uvVT.y = (PassDataCB.runtimeVTRealRect.y - input.wsPos.z) / PassDataCB.runtimeVTRealRect.w;
	uint2 pagePos = floor(uvVT * (float)PassDataCB.runtimeVTTileCountPerAxisInPage0Level);
	uint4 pageData = runtimeVTPageTableMap[pagePos.xy].rgba;

	float  tilePaddingSize = PassDataCB.tilePaddingSize;
	float  tileSizeNoPadding = PassDataCB.tileSizeNoPadding;
	float  tileSizeWithPadding = tileSizeNoPadding + tilePaddingSize * 2.0f;
	float2 inTileOffset = frac(uvVT * exp2(PassDataCB.runtimeVTMaxPageLevel - pageData.z));
	float2 uvAtlas = float2(0.0f, 0.0f);
	uvAtlas.x = ((float)pageData.x * tileSizeWithPadding + tilePaddingSize + inTileOffset.x * tileSizeNoPadding) / PassDataCB.runtimeVTAtlasSize;
	uvAtlas.y = ((float)pageData.y * tileSizeWithPadding + tilePaddingSize + inTileOffset.y * tileSizeNoPadding) / PassDataCB.runtimeVTAtlasSize;

	float3 currLodColor = GetLODColor(input.nodeLod);
	float3 runtimeVTAlbedo = runtimeVTAlbedoAtlas.SampleLevel(SamplerLinearWrap, uvAtlas, 0u).rgb;
	float3 runtimeVTNormal = runtimeVTNormalAtlas.SampleLevel(SamplerLinearWrap, uvAtlas, 0u).rgb;

	float3 pageLevelColor = float3(clamp(1.0f - pageData.z * 0.1f , 0.0f, 1.0f), 0.0f, 0.0f);


	p2o output;
	output.albedoMetalness  = float4(runtimeVTAlbedo, 0.0f);
	output.positionEmission = float4(input.wsPos, 0.0f);
	output.normalRoughness  = float4(input.terrainNormal.rgb, 1.0f);
	output.motionVector     = float4(velocity.xy, 0.0f, 0.0f);
	output.viewDepth        = input.vsPos.z;

	return output;
}

#endif