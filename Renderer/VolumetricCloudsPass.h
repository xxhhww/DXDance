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
			uint32_t blueNoise2DMapIndex;
			uint32_t blueNoise2DMapWidth;
			uint32_t blueNoise2DMapHeight;
			float pad1;
			uint32_t gBufferViewDepthMapIndex;
			uint32_t previousPassOutputMapIndex;	// ǰһ��Pass���������������Pass������������
			uint32_t mipLevel;
			uint32_t previousPassOutputWidth;
			uint32_t previousPassOutputHeight;
			float pad2;
			float pad3;
			// ���Ʋ���
			float cloudsBottomHeight = 1500.0f;
			float cloudsTopHeight = 4000.0f;
			float scatterForward = 0.5f;			// ��ǰɢ��[0, 0.99]
			float scatterForwardIntensity = 1.0f;	// ��ǰɢ��ǿ��[0, 1]
			float scatterBackward = 0.4f;			// ���ɢ��[0, 0.99]
			float scatterBackwardIntensity = 0.4f;	// ���ɢ��ǿ��[0, 1]
			float scatterBase = 0.2f;				// ����ɢ��[0, 1]
			float scatterMultiply = 0.7f;			// ��������[0, 1]
			uint32_t cloudRaymarchSteps = 64;		// ���߷��򲽽�����
			uint32_t lightRaymarchSteps = 8;		// ���շ��򲽽�����
			float darknessThreshold = 0.3f;			// ������ֵ
			float colorCentralOffset = 0.5f;		// �м���ɫƫ�� 
			Math::Vector4 colorBright { 1.0f, 1.0f, 1.0f, 1.0f };	// ������ɫ
			Math::Vector4 colorCentral{ 0.5f, 0.5f, 0.5f, 1.0f };	// �м���ɫ
			Math::Vector4 colorDark   { 0.1f, 0.1f, 0.1f, 1.0f };	// ������ɫ
		};

		VolumetricCloudsMainPassData volumetricCloudsMainPassData;

		TextureWrap weatherMap;

		inline static uint32_t smThreadSizeInGroup = 16u;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);
	};

}