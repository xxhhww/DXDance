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
		* 描述平截头的八个顶点
		*/
		struct FrustumCorners {
		public:
			Math::Vector3 nearCorners[4];	// 近处四个顶点
			Math::Vector3 farCorners[4];	// 远处四个顶点
		};

	public:
		ShadowSystem() = default;
		~ShadowSystem() = default;

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
		void UpdateCascadeShadowLightCameraMatrix(const GPUCamera& gpuCamera, const GPULight& sunLight);

		/*
		* 计算第i个级联层级的平截头体的八个顶点
		*/
		void SpawnWorldSpaceFrustumCornersByIndex(int32_t index, const GPUCamera& gpuCamera, FrustumCorners& currFrustumCorners);

	private:
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

		// 级联阴影的包围盒与光源空间变换矩阵
		std::vector<Math::BoundingBox> mCascadeBoundingBoxsInWorldSpace;
		std::vector<Math::Matrix4> mCascadeShadowLightCameraViewMatrixs;
		std::vector<Math::Matrix4> mCascadeShadowLightCameraProjectMatrixs;

		// 级联阴影的DepthTexture
		std::vector<Renderer::TextureWrap> mCascadeShadowDepthTextures;
	};

}