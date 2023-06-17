#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class DeferredLightPass {
	public:
		struct DeferredLightPassData {
			uint32_t gBufferAlbedoMetalnessMapIndex;
			uint32_t gBufferPositionEmissionMapIndex;
			uint32_t gBufferNormalRoughnessMapIndex;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t finalOutputMapIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		DeferredLightPassData deferredLightPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}