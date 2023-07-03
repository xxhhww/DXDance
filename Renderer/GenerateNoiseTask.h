#pragma once
#include "DirectStorage/dstorage.h"
#include "DirectXTex/DirectXTex.h"

#include "Renderer/RenderEngine.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	/*
	* ���������������������
	*/
	class GenerateNoiseTask {
	public:
		void Generate(CommandBuffer& commandBuffer, RenderContext& renderContext);

		void OnCompleted();

	private:
	};

}