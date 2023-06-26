#pragma once
#include "Renderer/RenderGraph.h"

namespace Renderer {

	class TAAPass {
	public:
		struct TAAPassData {
			uint32_t dispatchGroupCountX;
			uint32_t dispatchGroupCountY;
			uint32_t previousTAAOutputMapIndex;    // ��һ֡TAA��������
			uint32_t previousPassOutputMapIndex;   // ǰһ��Pass��������
			uint32_t gBufferMotionVectorMapIndex;
			uint32_t currentTAAOutputMapIndex;
			uint32_t isFirstFrame;
			float pad1;
		};

		TAAPassData taaPassData;

		inline static uint32_t smThreadSizeInGroup = 16u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}