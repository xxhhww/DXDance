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
		* �߼����£��������µ�GrassCluster���룬�ɵ�GrassCluster��������RenderEngine��Update��������
		*/
		void Update(const Math::Vector2& cameraPosition);

		void AddPass(RenderGraph& renderGraph);

	private:
		inline static float smGrassClusterMeterSize{ 32.0f };			// 32 * 32 / m2 Ϊһ��GrassCluster
		inline static int32_t smMaxGrassBladeCountPerAxis{ 256 };		// ÿ����Ⱥ��ÿһ�����ϵ�GrassBlade�ĸ���
		inline static int32_t smVisibleGrassClusterCountPerAxis{ 8 };	// �ɼ���GrassCluster
		inline static float smVisibleDistance{ smGrassClusterMeterSize * smVisibleGrassClusterCountPerAxis / 2.0f  - smGrassClusterMeterSize / 2.0f };

		std::unique_ptr<VegetationDataCache> mDataCacher;
		std::unique_ptr<VegetationVirtualTable> mVegetationVirtualTable;

		std::vector<GpuGrassCluster> mNeedBakedGpuGrassClusters;	// ��Ҫִ�к決������GrassCluster
		std::vector<GpuGrassCluster> mNeedCullGpuGrassClusters;		// ��Ҫִ���޳�������GrassCluster
	};

}