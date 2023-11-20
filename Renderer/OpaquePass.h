#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	/*
	* ��Ⱦͨ���Ĳ�͸������
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