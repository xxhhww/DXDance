#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class OceanPass {
	public:
		struct OceanBuilderData {
			Math::Vector4 WindAndSeed = Math::Vector4(0.2f, 0.4f, 0.0f, 0.0f);	// 风和随机种子 xy为风, zw为两个随机种子

			int N;					// FFT纹理大小
			float OceanLength;		// 海洋长度
			float A = 10.0f;		// phillips谱参数，影响波浪高度
			int Ns;					// Ns = pow(2,m-1); m为第几阶段

			float Lambda = -1.0f;		// 偏移影响
			float HeightScale = 1;		// 高度影响
			float BubblesScale = 1;		// 泡沫强度
			float BubblesThreshold = 1; // 泡沫阈值

			uint32_t rngState;			// 随机
			uint32_t gaussianRandomMapIndex;
			uint32_t heightSpectrumMapIndex;
			uint32_t displaceXSpectrumMapIndex;

			uint32_t displaceZSpectrumMapIndex;
			uint32_t displaceMapIndex;
			uint32_t tempInputMapIndex;
			uint32_t tempOutputMapIndex;
			uint32_t normalMapIndex;

			uint32_t bubblesMapIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct OceanRendererData {

            uint32_t skyViewLutIndex;
		};

	public:
		OceanBuilderData oceanBuilderData;
		OceanRendererData oceanRendererData;

		bool isInitialized{ false };
		inline static int smThreadSizeInGroup = 8u;
		inline static int smFFTPow = 10u;				// 生成海洋纹理大小2的次幂
		inline static int smFFTSize = pow(2, smFFTPow);	// fft纹理大小 = pow(2, fftPow)
		inline static int smMeshSize = 250;			//网格长宽数量
		inline static float smMeshLength = 10.0f;	//网格长度
		std::unique_ptr<Renderer::Mesh> gridMesh;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	};

}