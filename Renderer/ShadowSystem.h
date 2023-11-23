#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/BoundingBox.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/RootConstantsPerFrame.h"
#include "Renderer/GPUData.h"

namespace Renderer {

	class RenderEngine;

	class ShadowSystem {
	public:
		/*
		* ����ƽ��ͷ�İ˸�����
		*/
		struct FrustumCorners {
		public:
			Math::Vector3 nearCorners[4];	// �����ĸ�����
			Math::Vector3 farCorners[4];	// Զ���ĸ�����
		};

	public:
		ShadowSystem() = default;
		~ShadowSystem() = default;

		/*
		* ��ʼ��
		*/
		void Initialize(RenderEngine* renderEngine);

		/*
		* ���Pass
		* ��GBuffer����Ӱ�������������Ǵӹ�Դ��Ⱦ��Ӱ��ͼ
		*/
		void AddPass(RenderEngine* renderEngine);

		/*
		* ���¹�Դ����ľ���ֵ
		*/
		void Update(RenderEngine* renderEngine);
	
	private:
		/*
		* ���㼶����Ӱ�����㼶�Ĺ�Դ����ľ���ֵ
		*/
		void UpdateCascadeShadowLightCameraMatrix(const GPUCamera& gpuCamera, const GPULight& sunLight);

		/*
		* �����i�������㼶��ƽ��ͷ��İ˸�����
		*/
		void SpawnWorldSpaceFrustumCornersByIndex(int32_t index, const GPUCamera& gpuCamera, FrustumCorners& currFrustumCorners);

	private:
		// ������Ӱ����
		inline static int32_t smNumShadowCascades = 4;
		
		// ������Ӱ�ֱ���
		inline static uint32_t smCascadeShadowDepthTextureResolution = 2048u;

		// ������Ӱ�ľ���
		inline static float smCascadeShadowDistance = 180.0f;

		/*
		* ������Ӱ�ľ������
		* cascade 0 [0.0f, 0.07f]
		* cascade 1 [0.7f, 0.2f]
		* cascade 2 [0.2f, 0.45f]
		* cascade 3 [0.45f, 1.0f]
		*/
		inline static std::vector<float> smShadowCascadesSplitRatio = {
			0.07f, 0.13f, 0.25f, 0.55f
		};

		// ������Ӱ�İ�Χ�����Դ�ռ�任����
		std::vector<Math::BoundingBox> mCascadeBoundingBoxsInWorldSpace;
		std::vector<Math::Matrix4> mCascadeShadowLightCameraViewMatrixs;
		std::vector<Math::Matrix4> mCascadeShadowLightCameraProjectMatrixs;

		// ������Ӱ��DepthTexture
		std::vector<Renderer::TextureWrap> mCascadeShadowDepthTextures;
	};

}