#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	/*
	* ��Ⱦͨ���Ĳ�͸������
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