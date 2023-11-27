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
		* 描述平截头的八个顶点
		*/
		struct FrustumCorners {
		public:
			Math::Vector3 nearCorners[4];	// 近处四个顶点
			Math::Vector3 farCorners[4];	// 远处四个顶点
		};

		struct CascadeShadowCullingPassData {
		public:
			uint32_t  opaqueItemDataBufferIndex;
			uint32_t  opaqueItemIndirectDrawIndexedDataBufferIndex;
			uint32_t  opaqueItemNumsPerFrame;
			float     pad1;

			uint32_t  cascadeShadow0IndirectArgsIndex;					// 级联层级0的可视Item的间接绘制参数
			uint32_t  cascadeShadow1IndirectArgsIndex;
			uint32_t  cascadeShadow2IndirectArgsIndex;
			uint32_t  cascadeShadow3IndirectArgsIndex;

			std::array<Math::Vector4, 6> cascadeShadow0FrustumPlanes;	// 级联层级0的光源可视平截头体
			std::array<Math::Vector4, 6> cascadeShadow1FrustumPlanes;
			std::array<Math::Vector4, 6> cascadeShadow2FrustumPlanes;
			std::array<Math::Vector4, 6> cascadeShadow3FrustumPlanes;
		};

		struct CascadeShadowRedirectPassData {
		public:
			uint32_t redirectedIndirectArgsIndex;	// 需要重定向的IndirectArgs索引
			uint32_t passDataAddressUp;				// GPU存储数据是每4位一存储，因此8位的GPU地址数据需要拆分成上下两份
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
		* 初始化
		*/
		void Initialize(RenderEngine* renderEngine);

		/*
		* 添加Pass
		* 对GBuffer的阴影分量做处理，而非从光源渲染阴影贴图
		*/
		void AddPass(RenderEngine* renderEngine);

		/*
		* 更新光源相机的矩阵值
		*/
		void Update(RenderEngine* renderEngine);
	
	private:
		/*
		* 计算级联阴影各个层级的光源相机的矩阵值
		*/
		void UpdateCascadeShadowLightCameraMatrix(const GpuCameraData& gpuCamera, const GpuLightData& sunLight);

		/*
		* 计算第i个级联层级的平截头体的八个顶点
		*/
		void GetWorldSpaceFrustumCornersByIndex(int32_t index, const GpuCameraData& gpuCamera, FrustumCorners& currFrustumCorners);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u;
		// 级联阴影个数
		inline static int32_t smNumShadowCascades = 4;
		
		// 级联阴影分辨率
		inline static uint32_t smCascadeShadowDepthTextureResolution = 2048u;

		// 级联阴影的距离
		inline static float smCascadeShadowDistance = 180.0f;

		/*
		* 级联阴影的距离分配
		* cascade 0 [0.0f, 0.07f]
		* cascade 1 [0.7f, 0.2f]
		* cascade 2 [0.2f, 0.45f]
		* cascade 3 [0.45f, 1.0f]
		*/
		inline static std::vector<float> smShadowCascadesSplitRatio = {
			0.07f, 0.13f, 0.25f, 0.55f
		};

		// 级联阴影各个层级的平截头体平面与光源相机矩阵
		std::vector<Math::Matrix4> mCascadedLightCameraGpuVpMatrixs;					// 已转置
		std::vector<std::array<Math::Vector4, 6>> mCascadedFrustumPlanesInWorldSpace;

		// 级联阴影的DepthTexture
		std::vector<Renderer::TextureWrap> mCascadeShadowDepthTextures;

		CascadeShadowCullingPassData mCascadeShadowCullingPassData;
		CascadeShadowRedirectPassData mCascadeShadowRedirectPassData;
		CascadeShadowPassData mCascadeShadowPassData;
	};

}