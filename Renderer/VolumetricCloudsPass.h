#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"

namespace Renderer {
	class RenderEngine;

	class VolumetricCloudsPass {
	public:
		struct VolumetricCloudsMainPassData {
			// 资源索引
			uint32_t weatherMapIndex;
			uint32_t shapeNoiseMapIndex;
			uint32_t detailNoiseMapIndex;
			uint32_t blueNoise2DMapIndex;
			uint32_t blueNoise2DMapWidth;
			uint32_t blueNoise2DMapHeight;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t previousPassOutputMapIndex;	// 前一个Pass的输出结果，体积云Pass将结果输出其上
			uint32_t mipLevel;
			uint32_t previousPassOutputWidth;
			uint32_t previousPassOutputHeight;
			float    pad1;
			// 控制参数
			float cloudsBottomHeight = 2000.0f;						// 云层底部高度
			float cloudsLayHeight = 8000.0f;						// 云层的总层高
			float scatterForward = 0.5f;							// 向前散射[0, 0.99]
			float scatterForwardIntensity = 1.0f;					// 向前散射强度[0, 1]
			float scatterBackward = 0.4f;							// 向后散射[0, 0.99]
			float scatterBackwardIntensity = 0.4f;					// 向后散射强度[0, 1]
			float scatterBase = 0.2f;								// 基础散射[0, 1]
			float scatterMultiply = 0.7f;							// 基础乘数[0, 1]
			uint32_t cloudRaymarchSteps = 64;						// 视线方向步进次数
			uint32_t lightRaymarchSteps = 8;						// 光照方向步进次数
			float crispiness = 40.0f;
			float curliness = 0.1f;
			float coverage = 0.45f;
			float absorption = 0.003f;
			float densityFactor = 0.02f;
			float cloudType = 1.0f;
		};

		VolumetricCloudsMainPassData volumetricCloudsMainPassData;

		TextureWrap weatherMap;
		TextureWrap shapeNoiseMap;
		TextureWrap detailNoiseMap;

		inline static uint32_t smThreadSizeInGroup = 16u;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);
	};

}