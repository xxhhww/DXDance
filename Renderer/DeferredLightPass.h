#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class DeferredLightPass {
	public:
		struct DeferredLightPassData {
			uint32_t _GBufferAlbedoIndex;
			uint32_t _GBufferPositionIndex;
			uint32_t _GBufferNormalIndex;
			uint32_t _GBufferMREIndex;
			uint32_t _FinalOutputIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		DeferredLightPassData _DeferredLightPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;
	public:
		void AddPass(RenderGraph& renderGraph);
	};

}