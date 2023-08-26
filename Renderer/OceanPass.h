#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class OceanPass {
	public:
		struct OceanBuilderData {
			Math::Vector4 WindAndSeed = Math::Vector4(1.0f, 1.0f, 0.0f, 0.0f);	// ���������� xyΪ��, zwΪ�����������

			float WindScale = 30.0f;
			int N;					// FFT�����С
			float OceanLength;		// ���󳤶�
			float A = 73.0f;		// phillips�ײ�����Ӱ�첨�˸߶�

			int Ns;					// Ns = pow(2,m-1); mΪ�ڼ��׶�
			float Lambda = 8.0f;		// ƫ��Ӱ��
			float HeightScale = 28.8f;	// �߶�Ӱ��
			float BubblesScale = 1;		// ��ĭǿ��

			float BubblesThreshold = 0.86; // ��ĭ��ֵ
			float DeltaTime = 0.0f;
			uint32_t gaussianRandomMapIndex;
			uint32_t heightSpectrumMapIndex;

			uint32_t displaceXSpectrumMapIndex;
			uint32_t displaceZSpectrumMapIndex;
			uint32_t displaceMapIndex;
			uint32_t tempInputMapIndex;

			uint32_t tempOutputMapIndex;
			uint32_t oceanNormalMapIndex;
			uint32_t oceanBubblesMapIndex;
			float pad1;
		};

		struct OceanRendererData {
			Math::Vector4 oceanColorShallow = Math::Vector4{ 0.31f, 0.47f, 0.55f, 1.0f };
			Math::Vector4 oceanColorDeep = Math::Vector4{ 0.05f, 0.14f, 0.21f, 1.0f };
			Math::Vector4 bubblesColor = Math::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
			Math::Vector4 specular = Math::Vector4{ 0.39f, 0.39f, 0.39f, 1.0f };

			float gloss = 256.0f;
			float fresnelScale = 0.02f;
			float tessellationFactor = 1.0f;
			uint32_t oceanDisplaceMapIndex;

			uint32_t oceanNormalMapIndex;
			uint32_t oceanBubblesMapIndex;
            uint32_t skyViewLutIndex;
			float pad1;
		};

	public:
		OceanBuilderData oceanBuilderData;
		OceanRendererData oceanRendererData;

		bool isInitialized{ false };
		inline static int smThreadSizeInGroup = 8u;
		inline static int smFFTPow = 9u;				// ���ɺ��������С2�Ĵ���
		inline static int smFFTSize = pow(2, smFFTPow);	// fft�����С = pow(2, fftPow)
		inline static int smMeshSize = 100;			//���񳤿�����
		inline static float smMeshLength = 512.0f;	//���񳤶�
		std::unique_ptr<Renderer::Mesh> gridMesh;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	};

}