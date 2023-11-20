#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class DeferredLightPass {
	public:
		struct DeferredLightPassData {
			Math::Vector4 halton;

			uint32_t gBufferAlbedoMetalnessMapIndex;
			uint32_t gBufferPositionEmissionMapIndex;
			uint32_t gBufferNormalRoughnessMapIndex;
			uint32_t gBufferViewDepthMapIndex;

			uint32_t rngSeedMapIndex;
			uint32_t blueNoise3DMapIndex;
			uint32_t deferredLightshadingOutMapIndex;
			float pad1;

			uint32_t deferredLightshadingOutMapSizeX;
			uint32_t deferredLightshadingOutMapSizeY;
			float pad2;
			float pad3;
		};

	public:
		void AddPass(RenderGraph& renderGraph);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u;
		DeferredLightPassData deferredLightPassData;
	};

}