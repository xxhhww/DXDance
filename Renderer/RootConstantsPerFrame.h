#pragma once
#include "GPUCamera.h"

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
	};

}