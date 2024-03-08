#ifndef _TerrainQuadTreeBuilder__
#define _TerrainQuadTreeBuilder__

#include "TerrainHeader.hlsl"

struct PassData {
	float4 nodeEvaluationC;			// �û����ƵĽڵ�����ϵ��

	float2 terrainMeterSize;		// ������XZ�᷽��Ĵ�С(��)
	float  terrainHeightScale;		// Y�����Ŵ�С
	uint   useFrustumCull;

	uint   currPassLOD;
	uint   currNodeListIndex;
	uint   nextNodeListIndex;
	uint   finalNodeListIndex;

	uint   nodeDescriptorListIndex;
	uint   lodDescriptorListIndex;
	uint   culledPatchListIndex;
	uint   nodeGpuRuntimeStatesIndex;

	uint   nearCulledPatchListIndex;
	uint   farCulledPatchListIndex;
	uint   maxLod;
    uint   lodMapIndex;

	float4 runtimeVTRealRect;

	float sectorMeterSize;
	uint  useRenderCameraDebug;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Math/BoundingBox.hlsl"

// ����Node���������Node��ȫ��ID
uint GetGlobalNodeId(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];
	
	uint nodeCountPerRow = PassDataCB.terrainMeterSize.x / currLodDescriptor.nodeMeterSize;

    return currLodDescriptor.nodeStartOffset + nodeLoc.y * nodeCountPerRow + nodeLoc.x;
}

// ���Node����������ռ��µ������XZ����
float2 GetNodeWSPositionXZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];

	float nodeMeterSize     = currLodDescriptor.nodeMeterSize;
	float nodeCountPerAxis  = PassDataCB.terrainMeterSize.x / nodeMeterSize;
	float2 nodeWSPositionXZ = float2(0.0f, 0.0f);
	nodeWSPositionXZ.x = ((float)nodeLoc.x - (float)((nodeCountPerAxis - 1.0f) * 0.5f)) * nodeMeterSize;
	nodeWSPositionXZ.y = ((float)((nodeCountPerAxis - 1.0f) * 0.5f) - (float)nodeLoc.y) * nodeMeterSize;
    return nodeWSPositionXZ;
}

// ���Node����������ռ��µ������XYZ����
float3 GetNodeWSPositionXYZ(uint2 nodeLoc, uint lod) {
	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	// ��ȡ�ڵ�ȫ��ID
	uint globalID = GetGlobalNodeId(nodeLoc, lod);
	TerrainNodeDescriptor currNodeDescriptor = nodeDescriptorList[globalID];

	// ����XZ���ϵ���������
	float2 wsPositionXZ = GetNodeWSPositionXZ(nodeLoc, lod);
    float  wsPositionY  = (currNodeDescriptor.minHeight + currNodeDescriptor.maxHeight) * 0.5f * PassDataCB.terrainHeightScale;
	return float3(wsPositionXZ.x, wsPositionY, wsPositionXZ.y);
}

bool EvaluateNode(uint2 nodeLoc, uint lod) {
	// ��ǰ�����Ľڵ�Lod������С������Ҫ�ٽ��зָ��ж�
	if(lod == 0) { return false; }

	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[lod];

	// �����жϸýڵ���ӽڵ����Դ�Ƿ񱻼��أ�δ������ֱ�ӷ���
	uint2 childNodeLoc  = nodeLoc;
	uint  childNodeLod  = lod - 1;
	uint  childGlobalID = 0;

	// �����ӽڵ�
	childNodeLoc  = nodeLoc * 2u + uint2(0, 0);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// �����ӽڵ�
	childNodeLoc  = nodeLoc * 2u + uint2(1, 0);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }	

	// �����ӽڵ�
	childNodeLoc  = nodeLoc * 2u + uint2(0, 1);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// �����ӽڵ�
	childNodeLoc  = nodeLoc * 2u + uint2(1, 1);
	childGlobalID = GetGlobalNodeId(childNodeLoc, childNodeLod); 
	if(nodeDescriptorList[childGlobalID].tilePosX == 255) { return false; }

	// �Ե�ǰ�ڵ�ľ����������
    float3 wsPositionXYZ = GetNodeWSPositionXYZ(nodeLoc, lod);
	wsPositionXYZ.y = 0.0f;

	float3 wsCameraPosition = float3(0.0f, 0.0f, 0.0f);
	if(PassDataCB.useRenderCameraDebug) {
		wsCameraPosition = FrameDataCB.CurrentRenderCamera.Position.xyz;
	}
	else {
		wsCameraPosition = FrameDataCB.CurrentEditorCamera.Position.xyz;
	}
	wsCameraPosition.y = 0.0f;
    
	float dis = distance(wsCameraPosition, wsPositionXYZ.xyz);
    float f = dis / (currLodDescriptor.nodeMeterSize * PassDataCB.nodeEvaluationC.x);
    if(f < 1) { return true; }
    return false;
}

[numthreads(1, 1, 1)]
void TraverseQuadTree(uint3 DTid : SV_DispatchThreadID) {
	ConsumeStructuredBuffer<uint2> currNodeList  = ResourceDescriptorHeap[PassDataCB.currNodeListIndex];
	AppendStructuredBuffer<uint2>  nextNodeList  = ResourceDescriptorHeap[PassDataCB.nextNodeListIndex];
	AppendStructuredBuffer<uint3>  finalNodeList = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];

	RWStructuredBuffer<TerrainNodeGpuRuntimeState> nodeGpuRuntimeStates = ResourceDescriptorHeap[PassDataCB.nodeGpuRuntimeStatesIndex];

	// xzƽ���µĶ�ά����
	uint2 nodeLoc = currNodeList.Consume();
	// �жϵ�ǰ�ڵ��Ƿ�����Ĳ滮��
	bool needBranch = EvaluateNode(nodeLoc, PassDataCB.currPassLOD);
	// ��ȡ���νڵ��ȫ��ID
	uint nodeGlobalID = GetGlobalNodeId(nodeLoc, PassDataCB.currPassLOD); 

	if(needBranch) {
		// �Խڵ�����Ĳ滮��
		nextNodeList.Append(nodeLoc * 2);
		nextNodeList.Append(nodeLoc * 2 + uint2(1, 0));
		nextNodeList.Append(nodeLoc * 2 + uint2(0, 1));
		nextNodeList.Append(nodeLoc * 2 + uint2(1, 1));
		nodeGpuRuntimeStates[nodeGlobalID].branch = 1;
	}
	else {
		// ���Խڵ���л��֣���������ȷ����Ҫ��Ⱦ�Ľڵ�
		finalNodeList.Append(uint3(nodeLoc, PassDataCB.currPassLOD));
		nodeGpuRuntimeStates[nodeGlobalID].branch = 0;
	}
}

[numthreads(8, 8, 1)]
void BuildTerrainLodMap(uint3 DTid : SV_DispatchThreadID) {
	RWTexture2D<uint4> lodMap = ResourceDescriptorHeap[PassDataCB.lodMapIndex];

	StructuredBuffer<TerrainNodeGpuRuntimeState> nodeGpuRuntimeStates = ResourceDescriptorHeap[PassDataCB.nodeGpuRuntimeStatesIndex];

	// sectorָLod0�µĵ��νڵ�
	uint2 sectorLoc = DTid.xy;	
    for(int lod = PassDataCB.maxLod; lod >= 0; lod--){
		uint  powNumber = pow(2, lod);
        uint2 nodeLoc = sectorLoc / powNumber;
        uint  nodeGlobalID = GetGlobalNodeId(nodeLoc, lod);
        if(nodeGpuRuntimeStates[nodeGlobalID].branch == 0){
            lodMap[sectorLoc.xy].rgba = uint4(nodeLoc, lod, 0);
            return;
        }
    }
    lodMap[sectorLoc.xy].rgba = uint4(0, 0, 0, 0);
}

//����һ�����νڵ㸲�ǵ�Sector(ָLod0�µĵ��νڵ�)��Χ
int4 GetSectorBounds(uint3 nodeLoc){
    int  powNumber = pow(2, nodeLoc.z);
    int2 sectorLocMin = nodeLoc.xy * powNumber;
    return int4(sectorLocMin, sectorLocMin + powNumber - 1);
}

uint GetLod(uint2 sectorLoc){
	Texture2D<uint4> lodMap = ResourceDescriptorHeap[PassDataCB.lodMapIndex];
	uint sectorCountPerAxis = PassDataCB.terrainMeterSize.x / PassDataCB.sectorMeterSize;

    if(sectorLoc.x < 0 || sectorLoc.y < 0 || sectorLoc.x >= sectorCountPerAxis || sectorLoc.y >= sectorCountPerAxis){
        return 0;
    }
    return lodMap[sectorLoc.xy].b;
}

void SetLodTrans(inout RenderPatch patch, uint3 nodeLoc, uint2 patchOffset){
    uint lod = nodeLoc.z;
    uint4 sectorBounds = GetSectorBounds(nodeLoc);
    int4 lodTrans = int4(0, 0, 0, 0);

	// patchOffset����groupThreadID.xy���Ǵ����½ǿ�ʼ�����
	// sectorBounds.xy�������Ͻǵ�sector��sectorBounds.zw�������½ǵ�sector
    if(patchOffset.x == 0){
        // ���Ե
        lodTrans.x = GetLod(sectorBounds.xy + int2(-1, 0)) - lod;
    }

    if(patchOffset.y == 0){
        // �ϱ�Ե
        lodTrans.y = GetLod(sectorBounds.xy + int2(0, -1)) - lod;
    }

    if(patchOffset.x == PATCH_COUNT_PER_NODE_PER_AXIS - 1) {
        // �ұ�Ե
        lodTrans.z = GetLod(sectorBounds.zw + int2(1, 0)) - lod;
    }

    if(patchOffset.y == PATCH_COUNT_PER_NODE_PER_AXIS - 1) {
        // �±�Ե
        lodTrans.w = GetLod(sectorBounds.zw + int2(0, 1)) - lod;
    }

    patch.lodTrans = (uint4)max(0, lodTrans);
}

RenderPatch CreatePatch(uint3 nodeLoc, uint2 patchOffset) {
	uint2 nodeLocXY = nodeLoc.xy;
    uint  nodeLod   = nodeLoc.z;

	StructuredBuffer<TerrainNodeDescriptor> nodeDescriptorList = ResourceDescriptorHeap[PassDataCB.nodeDescriptorListIndex];
	StructuredBuffer<TerrainLodDescriptor>  lodDescriptorList  = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];

	// ��ȡ�ڵ�ȫ��ID
	uint globalID = GetGlobalNodeId(nodeLocXY, nodeLod);
	
	TerrainNodeDescriptor currNodeDescriptor = nodeDescriptorList[globalID];
	TerrainLodDescriptor  currLodDescriptor  = lodDescriptorList[nodeLod];
	
	// ��ȡPatch����
    float patchMeterSize = currLodDescriptor.nodeMeterSize / PATCH_COUNT_PER_NODE_PER_AXIS;
    
	// ��ȡ�ڵ����ĵ�XZ�����������
	float2 nodeWSPositionXZ = GetNodeWSPositionXZ(nodeLocXY, nodeLod);

	// ����Patch���ĵ�XZ�����������
    RenderPatch patch;
    patch.nodeLoc = nodeLoc;
	patch.patchOffset = patchOffset;
	patch.position.x = nodeWSPositionXZ.x + ((float)patchOffset.x - (float)((PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f)) * patchMeterSize;
    patch.position.y = nodeWSPositionXZ.y - ((float)patchOffset.y - (float)((PATCH_COUNT_PER_NODE_PER_AXIS - 1.0f) * 0.5f)) * patchMeterSize;
	patch.minmaxHeight = float2(currNodeDescriptor.minHeight, currNodeDescriptor.maxHeight) * PassDataCB.terrainHeightScale + float2(-5.0f, 5.0f);
	patch.pad1 = 0.0f;
	patch.pad2 = 0.0f;
	patch.pad3 = 0.0f;
	patch.lodTrans = uint4(0, 0, 0, 0);
	return patch;
}

// ����Patch�İ�Χ��
BoundingBox GetPatchBoundingBox(RenderPatch patch) {
	StructuredBuffer<TerrainLodDescriptor> lodDescriptorList = ResourceDescriptorHeap[PassDataCB.lodDescriptorListIndex];
	TerrainLodDescriptor currLodDescriptor = lodDescriptorList[patch.nodeLoc.z];

    float patchExtentSize = currLodDescriptor.nodeMeterSize / (PATCH_COUNT_PER_NODE_PER_AXIS * 2.0f);

    BoundingBox boundingBox;
    float4 minPosition = float4(0.0f, 0.0f, 0.0f, 0.0f); 
	float4 maxPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);

    minPosition.xz = patch.position - patchExtentSize;
    maxPosition.xz = patch.position + patchExtentSize;
    minPosition.y = patch.minmaxHeight.x;
    maxPosition.y = patch.minmaxHeight.y;

    boundingBox.minPosition = minPosition;
    boundingBox.maxPosition = maxPosition;
    return boundingBox;
}

bool Cull(BoundingBox boundingBox) {
	if(PassDataCB.useFrustumCull){
		float4 cameraPlanes[6];
		if(PassDataCB.useRenderCameraDebug) {
			cameraPlanes = FrameDataCB.CurrentRenderCamera.Planes;
		}
		else {
			cameraPlanes = FrameDataCB.CurrentEditorCamera.Planes;
		}
		if(FrustumCull(cameraPlanes, boundingBox)) {
			return true;
		}
	}
    return false;
}

[numthreads(8, 8, 1)]
void BuildPatches(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID) {
	StructuredBuffer<uint3>             finalNodeList       = ResourceDescriptorHeap[PassDataCB.finalNodeListIndex];
	AppendStructuredBuffer<RenderPatch> culledPatchList     = ResourceDescriptorHeap[PassDataCB.culledPatchListIndex];
	AppendStructuredBuffer<RenderPatch> nearCulledPatchList = ResourceDescriptorHeap[PassDataCB.nearCulledPatchListIndex];
	AppendStructuredBuffer<RenderPatch> farCulledPatchList  = ResourceDescriptorHeap[PassDataCB.farCulledPatchListIndex];

	uint3 nodeLoc = finalNodeList[groupID.x];
    uint2 patchOffset = groupThreadID.xy;
	patchOffset.y = (PATCH_COUNT_PER_NODE_PER_AXIS - 1) - patchOffset.y;

    // ����Patch
    RenderPatch patch = CreatePatch(nodeLoc, patchOffset);
	SetLodTrans(patch, nodeLoc, patchOffset);


	// ����Patch�İ�Χ��
	BoundingBox patchBoundingBox = GetPatchBoundingBox(patch);

	// �ü�Patch
	if(Cull(patchBoundingBox)) { return; }

    culledPatchList.Append(patch);

	// ����Patch�Ƿ���RuntimeVTRealRect��
	// RuntimeVTRealRect�����Ͻ�Ϊ��㣬����ת��Ϊ���½ǣ�ֻ��Ҫƫ��z�ᣬҲ����y����
	float4 runtimeVTRealRect = PassDataCB.runtimeVTRealRect;
	runtimeVTRealRect.y = runtimeVTRealRect.y - runtimeVTRealRect.w;

	//����Patch��Rect
	float2 patchLength = float2(patchBoundingBox.maxPosition.xz - patchBoundingBox.minPosition.xz);
	float4 patchRect = float4(patchBoundingBox.minPosition.xz, patchLength);

	if(IsRectInRect(patchRect, runtimeVTRealRect)) {
		// if(patch.lodTrans.x == 0 && patch.lodTrans.y == 0 && patch.lodTrans.z == 0 && patch.lodTrans.w == 0) { return; }
		nearCulledPatchList.Append(patch);
	}
	else {
		farCulledPatchList.Append(patch);
	}
}

#endif