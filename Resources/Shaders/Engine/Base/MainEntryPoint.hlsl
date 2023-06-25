#ifndef _MainEntryPoint__
#define _MainEntryPoint__

#include "Camera.hlsl"
#include "Light.hlsl"

struct FrameData {
    Camera CurrentEditorCamera;
    Camera PreviousEditorCamera;

    Camera CurrentRenderCamera;
    Camera PreviousRenderCamera;

    float2 FinalRTResolution;
    float2 FinalRTResolutionInv;
    uint   LightSize;
    float  pad1;
    float  pad2;
    float  pad3;
};

#define FrameDataType FrameData
#define LightDataType Light

#include "BaseEngineLayout.hlsl"

#endif