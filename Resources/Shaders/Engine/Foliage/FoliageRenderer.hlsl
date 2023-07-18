#ifndef _FoliageRenderer__
#define _FoliageRenderer__

#include "FoliageHelper.hlsl"

struct PassData {
	uint  placementBufferIndex;
	uint  rotateToCamera;		// Foliage����Ⱦʱ���Ƿ�ǿ�Ƴ��������
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/MathCommon.hlsl"

struct a2v {
    float4 lsPos    : POSITION;
    float2 uv       : TEXCOORD;
    float3 lsNormal : NORMAL;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD;
};

struct p2o {
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3;
	float  viewDepth        : SV_TARGET4;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	StructuredBuffer<Placement> culledPlacementList = ResourceDescriptorHeap[PassDataCB.placementBufferIndex];

	Placement placement = culledPlacementList[instanceID];
	float4x4  currModelTrans = placement.modelTrans;

    // ��ģ�ͱ任��������ȡ���ž���
    float4x4 scaleMat;
    float scaleX = sqrt(currModelTrans[0][0] * currModelTrans[0][0] + currModelTrans[0][1] * currModelTrans[0][1] + currModelTrans[0][2] * currModelTrans[0][2]);
    float scaleY = sqrt(currModelTrans[1][0] * currModelTrans[1][0] + currModelTrans[1][1] * currModelTrans[1][1] + currModelTrans[1][2] * currModelTrans[1][2]);
    float scaleZ = sqrt(currModelTrans[2][0] * currModelTrans[2][0] + currModelTrans[2][1] * currModelTrans[2][1] + currModelTrans[2][2] * currModelTrans[2][2]);
    
    scaleMat[0][0] = scaleX;
    scaleMat[0][1] = 0.0f;
    scaleMat[0][2] = 0.0f;
    scaleMat[0][3] = 0.0f;
    scaleMat[1][0] = 0.0f;
    scaleMat[1][1] = scaleY;
    scaleMat[1][2] = 0.0f;
    scaleMat[1][3] = 0.0f;
    scaleMat[2][0] = 0.0f;
    scaleMat[2][1] = 0.0f;
    scaleMat[2][2] = scaleZ;
    scaleMat[2][3] = 0.0f;
    scaleMat[3][0] = 0.0f;
    scaleMat[3][1] = 0.0f;
    scaleMat[3][2] = 0.0f;
    scaleMat[3][3] = 1.0f;

    // ��ģ�ͱ任��������ȡƽ�ƾ���
    float4x4 translateMat;
    translateMat[0][0] = 1.0f;
    translateMat[0][1] = 0.0f;
    translateMat[0][2] = 0.0f;
    translateMat[0][3] = 0.0f;
    translateMat[1][0] = 0.0f;
    translateMat[1][1] = 1.0f;
    translateMat[1][2] = 0.0f;
    translateMat[1][3] = 0.0f;
    translateMat[2][0] = 0.0f;
    translateMat[2][1] = 0.0f;
    translateMat[2][2] = 1.0f;
    translateMat[2][3] = 0.0f;
    translateMat[3][0] = currModelTrans[3][0];
    translateMat[3][1] = currModelTrans[3][1];
    translateMat[3][2] = currModelTrans[3][2];
    translateMat[3][3] = currModelTrans[3][3];

    // �����ܽ�Foliageǿ�Ƴ������������ת���󣬶��Ǵ�ģ�ͱ任�����н�����ȡ
    float3 cameraLookUp = FrameDataCB.CurrentRenderCamera.LookUp.xyz;
    float angle = atan2(cameraLookUp.x, cameraLookUp.z) * (180.0f / PI);
    angle *= 0.0174532925f;
    
    float4x4 rotationMat;
    rotationMat[0][0] = cos(angle);
    rotationMat[0][1] = 0.0f;
    rotationMat[0][2] = -sin(angle);
    rotationMat[0][3] = 0.0f;
    rotationMat[1][0] = 0.0f;
    rotationMat[1][1] = 1.0f;
    rotationMat[1][2] = 0.0f;
    rotationMat[1][3] = 0.0f;
    rotationMat[2][0] = sin(angle);
    rotationMat[2][1] = 0.0f;
    rotationMat[2][2] = cos(angle);
    rotationMat[2][3] = 0.0f;
    rotationMat[3][0] = 0.0f;
    rotationMat[3][1] = 0.0f;
    rotationMat[3][2] = 0.0f;
    rotationMat[3][3] = 1.0f;

    v2p output;
    float3 tempWsPos = input.lsPos;
    tempWsPos = mul(tempWsPos, scaleMat);
    if (PassDataCB.rotateToCamera > 0u) {
        tempWsPos = mul(tempWsPos, rotationMat);
    }
    tempWsPos = mul(tempWsPos, translateMat);

    // TODO ��Foliageʩ�ӷ�����Ӱ��

    // Ŀǰ����Foliage���������겻�����ı�(ʵ����ģ�ͱ任�е���ת���ֻ��������������ת���任)
	float3 currWsPos = tempWsPos;
	float3 prevWsPos = tempWsPos;

    float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// ǰһ֡��CsPos������Ҫ������һ֡�Ķ�������PS�м���ʱ�ټ�����һ֡��uv�������Ӷ���֤����motionVectorʱ��������
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

    output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;

    // ???
    float3 tempNormal = float3(0.0f, 1.0f, 0.0f);
    output.wsNormal = normalize(mul(float4(tempNormal, 0.0f), currModelTrans).xyz);
	output.uv = input.uv;

    return output;
}



#endif