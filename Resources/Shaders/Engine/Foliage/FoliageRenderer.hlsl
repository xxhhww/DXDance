#ifndef _FoliageRenderer__
#define _FoliageRenderer__

#include "FoliageHelper.hlsl"

struct PassData {
	uint   placementBufferIndex;
    uint   heightMapIndex;
	uint   normalMapIndex;
	uint   rotateToCamera;        // Foliage在渲染时，是否强制朝向摄像机
    float2 worldMeterSize;
    uint   heightScale;
    uint   placementSizePerAxis;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

struct a2v {
    float3 lsPos     : POSITION;
    float2 uv        : TEXCOORD;
    float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
    float4 userData  : USERDATA;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
    float3 wsNormal  : NORMAL0;
	float2 uv        : TEXCOORD;
    uint   albedoMapIndex   : ALBEDOINDEX;
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

    // 构建能将Foliage强制朝向摄像机的旋转矩阵，而非从模型变换矩阵中进行提取
    float3 cameraLookUp = FrameDataCB.CurrentEditorCamera.LookUp.xyz;
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
    float4 tempWsPos = float4(input.lsPos + placement.position.xyz, 1.0f);

    /*
    tempWsPos = mul(tempWsPos, scaleMat);
    if (PassDataCB.rotateToCamera > 0u) {
        tempWsPos = mul(tempWsPos, rotationMat);
    }
    tempWsPos = mul(tempWsPos, translateMat);
    */

    // TODO 对Foliage施加风力的影响

    // 目前假设Foliage的世界坐标不发生改变(实际上模型变换中的旋转部分会随着摄像机的旋转而变换)
	float3 currWsPos = tempWsPos.xyz;
	float3 prevWsPos = tempWsPos.xyz;

    float3 currVsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.View).xyz;
	float4 currCsPos = mul(float4(currWsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
	// 前一帧的CsPos，不需要加上上一帧的抖动，在PS中计算时再加上这一帧的uv抖动，从而保证计算motionVector时消除抖动
	float4 prevCsPos = mul(float4(prevWsPos, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);

    output.currCsPos = currCsPos;
	output.prevCsPos = prevCsPos;
	output.wsPos = currWsPos;
	output.vsPos = currVsPos;

    // ???
    float3 tempNormal = float3(0.0f, 1.0f, 0.0f);
    output.wsNormal = normalize(tempNormal.xyz);
	output.uv = input.uv;
    output.albedoMapIndex = placement.albedoMapIndex;

    return output;
}

p2o PSMain(v2p input) {
    
    // 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);

    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);

    float3 velocity = currUVSpacePos - prevUVSpacePos;

    p2o output;

	output.albedoMetalness  = float4(1.0f, 1.0f, 1.0f, 0.0f);
	output.positionEmission = float4(float3(0.0f, input.wsPos.y, 0.0f), 1.0f);
	output.normalRoughness  = float4(input.wsNormal, 1.0f);
	output.motionVector     = float4(velocity, 0.0f);
	output.viewDepth        = input.vsPos.z;

    return output;
}

#endif