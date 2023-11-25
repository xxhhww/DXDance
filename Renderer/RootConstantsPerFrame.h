#pragma once
#include "Renderer/GPUData.h"

namespace Renderer {

	/*
	* ÿһ֡ˢ�µ�����
	*/
	struct RootConstantsPerFrame {
	public:
		GpuCameraData currentEditorCamera;
		GpuCameraData previousEditorCamera;

		GpuCameraData currentRenderCamera;
		GpuCameraData previousRenderCamera;

		Math::Vector2 finalRTResolution;
		Math::Vector2 finalRTResolutionInv;
		
		Math::Vector4 windParameters;

		uint32_t lightSize;
		float deltaTime;
		float totalTime;
		float pad1;
	};

}