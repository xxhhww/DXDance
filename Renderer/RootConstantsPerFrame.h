#pragma once
#include "Renderer/GPUData.h"

namespace Renderer {

	/*
	* ÿһ֡ˢ�µ�����
	*/
	struct RootConstantsPerFrame {
	public:
		GPUCamera currentEditorCamera;
		GPUCamera previousEditorCamera;

		GPUCamera currentRenderCamera;
		GPUCamera previousRenderCamera;

		Math::Vector2 finalRTResolution;
		Math::Vector2 finalRTResolutionInv;
		uint32_t lightSize;
		float pad1;
		float pad2;
		float pad3;
	};

}