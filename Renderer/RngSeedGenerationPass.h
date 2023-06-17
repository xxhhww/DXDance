#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	class RngSeedGenerationPass {
	public:
		struct RngSeedGenerationPassData {
			uint32_t rngSeedMapIndex;
			uint32_t blueNoise3DMapSize;
			uint32_t blueNoise3DMapDepth;
			uint32_t currFrameNumber;
		};

		RngSeedGenerationPassData rngSeedGenerationPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}