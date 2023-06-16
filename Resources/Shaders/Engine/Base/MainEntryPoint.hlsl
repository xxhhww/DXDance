#ifndef _MainEntryPoint__
#define _MainEntryPoint__

#include "Camera.hlsl"
#include "Light.hlsl"

struct FrameData {
    Camera CurrentEditorCamera;
    Camera PreviousEditorCamera;

    Camera CurrentRenderCamera;
    Camera PreviousRenderCamera;
};

#define FrameDataType FrameData
#define LightDataType Light

#include "BaseEngineLayout.hlsl"

#endif