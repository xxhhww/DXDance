#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RenderEngine;

	class OceanPass {
	public:
		struct OceanBuilderData {
			Math::Vector4 WindAndSeed = Math::Vector4(0.2f, 0.4f, 0.0f, 0.0f);	// ���������� xyΪ��, zwΪ�����������

			int N;					// FFT�����С
			float OceanLength;		// ���󳤶�
			float A = 10.0f;		// phillips�ײ�����Ӱ�첨�˸߶�
			int Ns;					// Ns = pow(2,m-1); mΪ�ڼ��׶�

			float Lambda = -1.0f;		// ƫ��Ӱ��
			float HeightScale = 1;		// �߶�Ӱ��
			float BubblesScale = 1;		// ��ĭǿ��
			float BubblesThreshold = 1; // ��ĭ��ֵ

			uint32_t rngState;			// ���
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
		inline static int smFFTPow = 10u;				// ���ɺ��������С2�Ĵ���
		inline static int smFFTSize = pow(2, smFFTPow);	// fft�����С = pow(2, fftPow)
		inline static int smMeshSize = 250;			//���񳤿�����
		inline static float smMeshLength = 10.0f;	//���񳤶�
		std::unique_ptr<Renderer::Mesh> gridMesh;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	};

}