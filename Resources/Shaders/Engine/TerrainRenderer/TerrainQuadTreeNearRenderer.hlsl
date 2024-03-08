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
	float runtimeVTMaxPageLevel;						// 理论最高的PageLevel,而不是实际最高的PageLevel
	float tilePaddingSize;
	float tileSizeNoPadding;

	uint  lodMapIndex;
	float patchMeshGridSize;
	float sectorMeterSize;
	float pad3;
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

// 将边缘多余的顶点衔接到附近的顶点上
void VertexConnect(RenderPatch patch, inout uint2 vertexIndex, inout float3 vertexPos) {
	uint4 lodTrans = patch.lodTrans;

	// 左边缘
    uint lodDelta = lodTrans.x;
    if(lodDelta > 0 && vertexIndex.x == 0){
        uint gridStripCount = pow(2, lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex != 0){
            vertexPos.z -= PassDataCB.patchMeshGridSize * modIndex;
			vertexIndex.y += modIndex;
            return;
        }
    }

	// 上边缘
    lodDelta = lodTrans.y;
    if(lodDelta > 0 && vertexIndex.y == 0){
        uint gridStripCount = pow(2, lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex != 0){
            vertexPos.x -= PassDataCB.patchMeshGridSize * modIndex;
			vertexIndex.x -= modIndex;
            return;
        }
    }

	// 右边缘
    lodDelta = lodTrans.z;
    if(lodDelta > 0 && vertexIndex.x == PassDataCB.terrainPatchVertexCountPerAxis - 1){
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex != 0){
            vertexPos.z -= PassDataCB.patchMeshGridSize * modIndex;
			vertexIndex.y += modIndex;
            return;
        }
    }

	// 下边缘
    lodDelta = lodTrans.w;
    if(lodDelta > 0 && vertexIndex.y == PassDataCB.terrainPatchVertexCountPerAxis - 1){
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex != 0){
            vertexPos.x -= PassDataCB.patchMeshGridSize * modIndex;
			vertexIndex.x -= modIndex;
            return;
        }
    }
}

// 将边缘顶点升格至相邻更高的LodLevel
void VertexUpgrade(RenderPatch patch, uint2 vertexIndex, float3 vertexPos, inout uint3 nodeLoc) {
	Texture2D<uint4> terrainLodMap = ResourceDescriptorHeap[PassDataCB.lodMapIndex];

	// 定位当前顶点所处的sectorLoc(LodMap中的坐标)

	// 1.计算顶点所属的nodeLoc对应的sectorLoc
	int  powNumber = pow(2, nodeLoc.z);
	int2 sectorLoc = nodeLoc.xy * powNumber;

	// 2.根据patchOffset计算初步顶点偏移
	int2 patchStartVertexIndexFromSectorLoc = patch.patchOffset * (PassDataCB.terrainPatchVertexCountPerAxis - 1);

	// 3.顶点总偏移量
	int2 vertexIndexFromSectorLoc = patchStartVertexIndexFromSectorLoc + vertexIndex;

	// 4.根据顶点总偏移量，计算出世界坐标下的偏移米数
	float  vertexSpaceInSectorInMeterSize   = PassDataCB.sectorMeterSize / (float)(PATCH_COUNT_PER_NODE_PER_AXIS * (PassDataCB.terrainPatchVertexCountPerAxis - 1));
	float  vertexSpaceInCurrLodInMeterSize  = vertexSpaceInSectorInMeterSize * powNumber;

	float2 vertexOffsetInCurrLodInMeterSize = vertexIndexFromSectorLoc * vertexSpaceInCurrLodInMeterSize;
	int2   vertexOffsetInSectorLoc = (int2)((vertexOffsetInCurrLodInMeterSize / PassDataCB.sectorMeterSize) - 0.01f);

	int2 currSectorLoc = sectorLoc + vertexOffsetInSectorLoc;

	uint4 lodTrans = patch.lodTrans;
	// 左边缘
    uint lodDelta = lodTrans.x;
	uint maxDelta = 0;

    if(lodDelta > 0 && vertexIndex.x == 0 && lodDelta > maxDelta){
		maxDelta = lodDelta;
        uint gridStripCount = pow(2, lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex == 0){
			uint4 lodMapData = terrainLodMap[int2(currSectorLoc.x - 1, currSectorLoc.y)];
			nodeLoc.xyz = lodMapData.rgb;
        }
    }


	// 上边缘
    lodDelta = lodTrans.y;
    if(lodDelta > 0 && vertexIndex.y == 0 && lodDelta > maxDelta){
		maxDelta = lodDelta;
        uint gridStripCount = pow(2, lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex == 0){
			uint4 lodMapData = terrainLodMap[int2(currSectorLoc.x, currSectorLoc.y - 1)];
			nodeLoc.xyz = lodMapData.rgb;
        }
    }

	// 右边缘
    lodDelta = lodTrans.z;
    if(lodDelta > 0 && vertexIndex.x == PassDataCB.terrainPatchVertexCountPerAxis - 1 && lodDelta > maxDelta){
		maxDelta = lodDelta;
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex == 0){
			uint4 lodMapData = terrainLodMap[int2(currSectorLoc.x + 1, currSectorLoc.y)];
			nodeLoc.xyz = lodMapData.rgb;
        }
    }

	// 下边缘
    lodDelta = lodTrans.w;
    if(lodDelta > 0 && vertexIndex.y == PassDataCB.terrainPatchVertexCountPerAxis - 1 && lodDelta > maxDelta){
		maxDelta = lodDelta;
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex == 0){
			uint4 lodMapData = terrainLodMap[int2(currSectorLoc.x, currSectorLoc.y + 1)];
			nodeLoc.xyz = lodMapData.rgb;
        }
    }
}

// 根据Node的索引获得Node的全局ID
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
	uint3  nodeLoc = renderPatch.nodeLoc;

	float3 vertexPos = input.lsPos;
	uint2  vertexIndex = (uint2)input.color;

	VertexConnect(renderPatch, vertexIndex, vertexPos);
	VertexUpgrade(renderPatch, vertexIndex, vertexPos, nodeLoc);


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

	float2 vertexPosXZInWorld = vertexPos.xz * pow(2, renderPatch.nodeLoc.z);
	vertexPosXZInWorld += renderPatch.position;

	float  nodeMeterSize = currLodDescriptor.nodeMeterSize;
	float2 absCurrNodeZS = nodeLoc.xy * nodeMeterSize;
	float2 absVertexPosXZInWorld = float2(vertexPosXZInWorld.x + 4096.0f, 4096.0f - vertexPosXZInWorld.y);
	float2 vertexPosXZInNode = absVertexPosXZInWorld - absCurrNodeZS;
	float2 finalfinal = (vertexPosXZInNode / nodeMeterSize) * 64.0f;


	/*
	// 第二步，计算当前Patch在Node对应的地形纹理中的索引偏移
	uint patchXIndexInTile = renderPatch.patchOffset.x * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.x);
	uint patchYIndexInTile = renderPatch.patchOffset.y * PassDataCB.terrainPatchVertexCountPerAxis - (renderPatch.patchOffset.y);

	// 第三步，计算当前顶点在Patch中的索引偏移
	uint vertexXIndexInPatch = (uint)input.color.x;
	uint vertexYIndexInPatch = (uint)input.color.y;

	uint2 currIndex;
	currIndex.x = tileXIndexInAtlas + patchXIndexInTile + vertexXIndexInPatch;
	currIndex.y = tileYIndexInAtlas + patchYIndexInTile + vertexYIndexInPatch;
	*/

	uint2 currIndex;
	currIndex.x = tileXIndexInAtlas + (uint)finalfinal.x;
	currIndex.y = tileYIndexInAtlas + (uint)finalfinal.y;








	v2p output;

	// 对地形纹理图形(heightMap albedoMap normalMap)进行采样
	output.terrainHeight = terrainHeightMapAtlas.mips[0][currIndex.xy].r;
	output.terrainAlbedo = terrainAlbedoMapAtlas.mips[0][currIndex.xy].rgb;
	output.terrainNormal = terrainNormalMapAtlas.mips[0][currIndex.xy].rgb;

	output.terrainNormal.xz = output.terrainNormal.xy * 2.0f - 1.0f;
    output.terrainNormal.y  = sqrt(max(0u, 1u - dot(output.terrainNormal.xz, output.terrainNormal.xz)));
	output.terrainNormal = normalize(output.terrainNormal);

	vertexPos.xz *= pow(2, renderPatch.nodeLoc.z);
	vertexPos.xz += renderPatch.position;
	vertexPos.y = output.terrainHeight * PassDataCB.terrainHeightScale;

	// 地形位置不会变化，因此currWsPos与prevWsPos是一样的
	float3 currWsPos = vertexPos;
	float3 prevWsPos = vertexPos;

	float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);


	output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;
	output.nodeLod = renderPatch.nodeLoc.z;
	return output;	
}

p2o PSMain(v2p input) {
	Texture2D runtimeVTAlbedoAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTAlbedoAtlasIndex];
	Texture2D runtimeVTNormalAtlas  = ResourceDescriptorHeap[PassDataCB.runtimeVTNormalAtlasIndex];
	Texture2D<uint4> runtimeVTPageTableMap = ResourceDescriptorHeap[PassDataCB.runtimeVTPageTableMapIndex];

	// 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);
    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);
    float3 velocity = currUVSpacePos - prevUVSpacePos;

	// 计算uvVT
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