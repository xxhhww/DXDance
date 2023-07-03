#pragma once
#include "DirectStorage/dstorage.h"
#include "DirectXTex/DirectXTex.h"

#include "Renderer/RenderEngine.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	/*
	* 生成噪声纹理的离线任务
	*/
	class GenerateNoiseTask {
	public:
		void Generate(CommandBuffer& commandBuffer, RenderContext& renderContext);

		void OnCompleted();

	private:
	};

}