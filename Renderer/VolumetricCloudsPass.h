#pragma once
#include <DirectStorage/dstorage.h>
#include "Renderer/RenderGraph.h"

namespace Renderer {
	class RenderEngine;

	class VolumetricCloudsPass {
	public:
		struct VolumetricCloudsMainPassData {
			// 资源索引
			uint32_t weatherMapIndex;
			uint32_t cloudMapIndex;
			uint32_t worleyMapIndex;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t previousPassOutputMapIndex;	// 前一个Pass的输出结果，体积云Pass将结果输出其上
			uint32_t mipLevel;
			uint32_t previousPassOutputWidth;
			uint32_t previousPassOutputHeight;
			// 控制参数
			float cloudsBottomHeight = 3000.0f;
			float cloudsTopHeight = 10000.0f;
			float crispiness = 43.0f;
			float curliness = 3.6f;
			float coverage = 0.505f;
			float cloudType = 1.0f;
			float absorption = 0.003f;
			float densityFactor = 0.015f;
		};

		VolumetricCloudsMainPassData volumetricCloudsMainPassData;

		std::unique_ptr<Renderer::Texture> weatherMap;
		std::unique_ptr<Renderer::Texture> cloudMap;
		std::unique_ptr<Renderer::Texture> worleyMap;

		inline static uint32_t smThreadSizeInGroup = 16u;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);
	};

}