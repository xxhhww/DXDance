#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"

namespace Renderer {
	class RenderEngine;

	class VolumetricCloudsPass {
	public:
		struct VolumetricCloudsMainPassData {
			// ��Դ����
			uint32_t weatherMapIndex;
			uint32_t shapeNoiseMapIndex;
			uint32_t detailNoiseMapIndex;
			uint32_t blueNoise2DMapIndex;
			uint32_t blueNoise2DMapWidth;
			uint32_t blueNoise2DMapHeight;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t previousPassOutputMapIndex;	// ǰһ��Pass���������������Pass������������
			uint32_t mipLevel;
			uint32_t previousPassOutputWidth;
			uint32_t previousPassOutputHeight;
			float    pad1;
			// ���Ʋ���
			float cloudsBottomHeight = 2000.0f;						// �Ʋ�ײ��߶�
			float cloudsLayHeight = 8000.0f;						// �Ʋ���ܲ��
			float scatterForward = 0.5f;							// ��ǰɢ��[0, 0.99]
			float scatterForwardIntensity = 1.0f;					// ��ǰɢ��ǿ��[0, 1]
			float scatterBackward = 0.4f;							// ���ɢ��[0, 0.99]
			float scatterBackwardIntensity = 0.4f;					// ���ɢ��ǿ��[0, 1]
			float scatterBase = 0.2f;								// ����ɢ��[0, 1]
			float scatterMultiply = 0.7f;							// ��������[0, 1]
			uint32_t cloudRaymarchSteps = 64;						// ���߷��򲽽�����
			uint32_t lightRaymarchSteps = 8;						// ���շ��򲽽�����
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