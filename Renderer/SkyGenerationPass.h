#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	class SkyGenerationPass {
	public:
		struct SkyGenerationPassData {
			Math::Vector3 sunDirection;
			uint32_t      skyLuminanceMapIndex;
			uint32_t      skyLuminanceMapSizeX;
			uint32_t      skyLuminanceMapSizeY;
			uint32_t      dispatchGroupCountX;
			uint32_t      dispatchGroupCountY;
			GPUArHosekSkyModelState skyStateR;
			GPUArHosekSkyModelState skyStateG;
			GPUArHosekSkyModelState skyStateB;
		};

		SkyGenerationPassData skyGenerationPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}
