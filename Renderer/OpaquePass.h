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
			uint32_t opaqueItemIndirectArgsIndex;
			uint32_t opaqueItemDataArrayIndex;
			float pad1;
			float pad2;
		};

	public:
		void AddPass(RenderGraph& renderGraph);

	private:
		GpuCullingPassData gpuCullingPassData;
	};

}