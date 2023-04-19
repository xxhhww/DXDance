#ifndef _MainEntryPoint__
#define _MainEntryPoint__

#include "Camera.hlsl"

struct FrameData {
    Camera CurrentEditorCamera;
    Camera PreviousEditorCamera;

    Camera CurrentRenderCamera;
    Camera PreviousRenderCamera;
};

#define FrameDataType FrameData

#include "BaseEngineLayout.hlsl"

#endif