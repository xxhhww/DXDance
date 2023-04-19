#pragma once
#include "GPUCamera.h"

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
	};

}