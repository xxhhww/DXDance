#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class DeferredLightPass {
	public:
		struct DeferredLightPassData {
			Math::Vector4 halton;
			uint32_t rngSeedMapIndex;
			uint32_t blueNoise3DMapIndex;
			uint32_t skyLuminanceMapIndex;
			uint32_t gBufferAlbedoMetalnessMapIndex;
			uint32_t gBufferPositionEmissionMapIndex;
			uint32_t gBufferNormalRoughnessMapIndex;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t finalOutputMapIndex;
			uint32_t finalOutputMapSizeX;
			uint32_t finalOutputMapSizeY;
			float pad1;
			float pad2;
		};

		DeferredLightPassData deferredLightPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}