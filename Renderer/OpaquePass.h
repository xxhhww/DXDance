#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	/*
	* 渲染通常的不透明物体
	*/
	class OpaquePass {
	public:
		void AddPreDepthPass(RenderGraph& renderGraph);
		
		void AddShadowPass(RenderGraph& renderGraph);

		void AddForwardPlusPass(RenderGraph& renderGraph);

		void AddGBufferPass(RenderGraph& renderGraph);

	private:
	};

}