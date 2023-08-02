#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Model.h"

namespace Renderer {

	class RenderEngine;

	class FoliagePass {
	public:
		struct FoliagePassData {
			uint32_t placementBufferIndex;
			uint32_t heightMapIndex;
			uint32_t normalMapIndex;
			uint32_t rotateToCamera = 1u;	// Foliage在渲染时，是否强制朝向摄像机
			Math::Vector2 worldMeterSize{ 5120u, 5120u };					// 世界在XZ轴方向的大小(米)
			uint32_t heightScale{ 4096u };
			uint32_t placementSizePerAxis;
			uint32_t useFrustumCull = 1u;
			float    pad1;
			float    pad2;
			float    pad3;
		};

		struct Placement {
			// 基础信息
			Math::Vector3 position;		// 安置点的坐标
			Math::Vector2 facing;		// 安置点的朝向
			uint32_t      type;			// 安置物类型(Grass / Tree / Stone)
			uint32_t      lod;			// LOD级别
			float         height;		// ?

			// 材质信息...
		};

		FoliagePassData foliagePassData;

		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smPlacementSizePerAxis = 4096u;		// 每个轴的安置点的个数
		inline static uint32_t smMaxPlacementSize = 2048u * 2048u;	// 安置点的个数的最大值															// 安置点数组

		std::unique_ptr<Mesh>  foliageMesh;

		TextureWrap			   foliageAlbedoMap;
		TextureWrap            foliageNormalMap;
	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}