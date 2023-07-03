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
			uint32_t blueNoise2DMapIndex;
			uint32_t blueNoise2DMapWidth;
			uint32_t blueNoise2DMapHeight;
			float pad1;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t previousPassOutputMapIndex;	// 前一个Pass的输出结果，体积云Pass将结果输出其上
			uint32_t mipLevel;
			uint32_t previousPassOutputWidth;
			uint32_t previousPassOutputHeight;
			float pad2;
			float pad3;
			// 控制参数
			float cloudsBottomHeight = 1500.0f;
			float cloudsTopHeight = 4000.0f;
			float scatterForward = 0.5f;			// 向前散射[0, 0.99]
			float scatterForwardIntensity = 1.0f;	// 向前散射强度[0, 1]
			float scatterBackward = 0.4f;			// 向后散射[0, 0.99]
			float scatterBackwardIntensity = 0.4f;	// 向后散射强度[0, 1]
			float scatterBase = 0.2f;				// 基础散射[0, 1]
			float scatterMultiply = 0.7f;			// 基础乘数[0, 1]
			uint32_t cloudRaymarchSteps = 64;		// 视线方向步进次数
			uint32_t lightRaymarchSteps = 8;		// 光照方向步进次数
			float darknessThreshold = 0.3f;			// 暗部阈值
			float colorCentralOffset = 0.5f;		// 中间颜色偏移 
			Math::Vector4 colorBright { 1.0f, 1.0f, 1.0f, 1.0f };	// 亮面颜色
			Math::Vector4 colorCentral{ 0.5f, 0.5f, 0.5f, 1.0f };	// 中间颜色
			Math::Vector4 colorDark   { 0.1f, 0.1f, 0.1f, 1.0f };	// 暗面颜色
		};

		VolumetricCloudsMainPassData volumetricCloudsMainPassData;

		TextureWrap weatherMap;

		inline static uint32_t smThreadSizeInGroup = 16u;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);
	};

}