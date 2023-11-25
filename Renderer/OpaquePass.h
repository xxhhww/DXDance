#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	/*
	* 渲染通常的不透明物体
	*/
	class OpaquePass {
	public:
		struct GpuCullingPassData {
		public:
			uint32_t deferredItemDataBufferIndex;
			uint32_t deferredItemIndirectDrawIndexedDataBufferIndex;
			uint32_t culledDeferredItemIndirectArgsIndex;
			uint32_t itemNumsPerFrame;
		};

	public:
		void AddPass(RenderGraph& renderGraph);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u;

		GpuCullingPassData mGpuCullingPassData;
	};

}