#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class ToneMappingPass {
	public:
		struct ToneMappingPassData {
			uint32_t  inputMapIndex;
			uint32_t  outputMapIndex;
			bool      isHDREnabled;
			float     displayMaxLuminance;
			GPUGTTonemappingParameters tonemappingParams;
		};

		ToneMappingPassData toneMappingPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;

	public:
		void AddPass(RenderGraph& renderGraph);

		void AddForwardPlusPass(RenderGraph& renderGraph);
	};

}