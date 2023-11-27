#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Frustum.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/RootConstantsPerFrame.h"
#include "Renderer/GPUData.h"

namespace Renderer {

	class RenderEngine;

	class CascadeShadowPass {
	public:
		/*
		* ����ƽ��ͷ�İ˸�����
		*/
		struct FrustumCorners {
		public:
			Math::Vector3 nearCorners[4];	// �����ĸ�����
			Math::Vector3 farCorners[4];	// Զ���ĸ�����
		};

		struct CascadeShadowCullingPassData {
		public:
			uint32_t  opaqueItemDataBufferIndex;
			uint32_t  opaqueItemIndirectDrawIndexedDataBufferIndex;
			uint32_t  opaqueItemNumsPerFrame;
			float     pad1;

			uint32_t  cascadeShadow0IndirectArgsIndex;					// �����㼶0�Ŀ���Item�ļ�ӻ��Ʋ���
			uint32_t  cascadeShadow1IndirectArgsIndex;
			uint32_t  cascadeShadow2IndirectArgsIndex;
			uint32_t  cascadeShadow3IndirectArgsIndex;

			std::array<Math::Vector4, 6> cascadeShadow0FrustumPlanes;	// �����㼶0�Ĺ�Դ����ƽ��ͷ��
			std::array<Math::Vector4, 6> cascadeShadow1FrustumPlanes;
			std::array<Math::Vector4, 6> cascadeShadow2FrustumPlanes;
			std::array<Math::Vector4, 6> cascadeShadow3FrustumPlanes;
		};

		struct CascadeShadowRedirectPassData {
		public:
			uint32_t redirectedIndirectArgsIndex;	// ��Ҫ�ض����IndirectArgs����
			uint32_t passDataAddressUp;				// GPU�洢������ÿ4λһ�洢�����8λ��GPU��ַ������Ҫ��ֳ���������
			uint32_t passDataAddressDown;
			float pad1;
		};

		struct CascadeShadowPassData {
		public:
			Math::Matrix4 vpMatrix;
		};

	public:
		CascadeShadowPass() = default;
		~CascadeShadowPass() = default;

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
		void UpdateCascadeShadowLightCameraMatrix(const GpuCameraData& gpuCamera, const GpuLightData& sunLight);

		/*
		* �����i�������㼶��ƽ��ͷ��İ˸�����
		*/
		void GetWorldSpaceFrustumCornersByIndex(int32_t index, const GpuCameraData& gpuCamera, FrustumCorners& currFrustumCorners);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u;
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

		// ������Ӱ�����㼶��ƽ��ͷ��ƽ�����Դ�������
		std::vector<Math::Matrix4> mCascadedLightCameraGpuVpMatrixs;					// ��ת��
		std::vector<std::array<Math::Vector4, 6>> mCascadedFrustumPlanesInWorldSpace;

		// ������Ӱ��DepthTexture
		std::vector<Renderer::TextureWrap> mCascadeShadowDepthTextures;

		CascadeShadowCullingPassData mCascadeShadowCullingPassData;
		CascadeShadowRedirectPassData mCascadeShadowRedirectPassData;
		CascadeShadowPassData mCascadeShadowPassData;
	};

}