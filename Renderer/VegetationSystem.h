#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"
#include "Renderer/VegetationVirtualTable.h"

namespace Renderer {

	struct GpuGrassCluster {
	public:
		Math::Vector4 grassClusterRect;
		uint32_t grassBladeBufferIndex;

	public:
		inline GpuGrassCluster(const Math::Vector4& _GrassClusterRect, uint32_t _GrassBladeBufferIndex)
		: grassClusterRect(_GrassClusterRect)
		, grassBladeBufferIndex(_GrassBladeBufferIndex) {}

		~GpuGrassCluster() = default;
	};

	class VegetationSystem {
	public:
		void Initialize(RenderEngine* renderEngine);

		/*
		* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出，由RenderEngine的Update函数调用
		*/
		void Update(const Math::Vector2& cameraPosition);

		void AddPass(RenderGraph& renderGraph);

	private:
		inline static float smGrassClusterMeterSize{ 32.0f };			// 32 * 32 / m2 为一个GrassCluster
		inline static int32_t smMaxGrassBladeCountPerAxis{ 256 };		// 每个草群的每一个轴上的GrassBlade的个数
		inline static int32_t smVisibleGrassClusterCountPerAxis{ 8 };	// 可见的GrassCluster
		inline static float smVisibleDistance{ smGrassClusterMeterSize * smVisibleGrassClusterCountPerAxis / 2.0f  - smGrassClusterMeterSize / 2.0f };

		std::unique_ptr<VegetationDataCache> mDataCacher;
		std::unique_ptr<VegetationVirtualTable> mVegetationVirtualTable;

		std::vector<GpuGrassCluster> mNeedBakedGpuGrassClusters;	// 需要执行烘焙操作的GrassCluster
		std::vector<GpuGrassCluster> mNeedCullGpuGrassClusters;		// 需要执行剔除操作的GrassCluster
	};

}