#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	class TAAPass {
	public:
		struct TAAPassData {
			uint32_t dispatchGroupCountX;
			uint32_t dispatchGroupCountY;
			uint32_t previousTAAOutputMapIndex;    // 上一帧TAA的输出结果
			uint32_t previousPassOutputMapIndex;   // 前一个Pass的输出结果
			uint32_t motionMapIndex;
			uint32_t outputMapIndex;
			float pad1;
			float pad2;
		};

		TAAPassData taaPassData;

		inline static uint32_t smThreadSizeInGroup = 16u;

	private:
		void AddPass(RenderGraph& renderGraph);
	};

}