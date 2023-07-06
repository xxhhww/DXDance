#pragma once
#include "Renderer/GPUData.h"

namespace Renderer {

	/*
	* 每一帧刷新的数据
	*/
	struct RootConstantsPerFrame {
	public:
		GPUCamera currentEditorCamera;
		GPUCamera previousEditorCamera;

		GPUCamera currentRenderCamera;
		GPUCamera previousRenderCamera;

		Math::Vector2 finalRTResolution;
		Math::Vector2 finalRTResolutionInv;
		
		Math::Vector4 windParameters;

		uint32_t lightSize;
		float deltaTime;
		float totalTime;
		float pad1;
	};

}