#ifndef _MainEntryPoint__
#define _MainEntryPoint__

#include "Camera.hlsl"
#include "Light.hlsl"
#include "GpuData.hlsl"

struct FrameData {
    Camera CurrentEditorCamera;
    Camera PreviousEditorCamera;

    Camera CurrentRenderCamera;
    Camera PreviousRenderCamera;

    float2 FinalRTResolution;
    float2 FinalRTResolutionInv;

    float4 WindParameters;

    uint   LightSize;
    float  DeltaTime;
    float  TotalTime;
    float  pad3;
};

#define FrameDataType FrameData
#define LightDataType Light
#define ItemDataType  ItemData

#include "BaseEngineLayout.hlsl"

#endif