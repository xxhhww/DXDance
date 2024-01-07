#pragma once
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class TerrainRenderer;

	class TerrainPipelinePass {
	public:
		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC;	// �û����ƵĽڵ�����ϵ��

			Math::Vector2 terrainMeterSize;	// ������XZ�᷽��Ĵ�С(��)
			float terrainHeightScale;
			uint32_t currPassLOD;

			uint32_t consumeNodeListIndex;
			uint32_t appendNodeListIndex;
			uint32_t finalNodeListIndex;
			uint32_t nodeDescriptorListIndex;

			uint32_t lodDescriptorListIndex;
			uint32_t culledPatchListIndex;
			uint32_t minmaxHeightMapIndex;
			uint32_t useFrustumCull{ 0u };

			uint32_t nearbyNodeListIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct TerrainRendererPassData {
			// x: page table size
			// y: virtual texture size
			// z: max mipmap level
			// w: mipmap level bias
			Math::Vector4 vtFeedbackParams{};
			Math::Vector4 vtRealRect{};
			// x: padding size
			// y: tileSize
			// z: physical texture size x
			// w: physical texture size y
			Math::Vector4 vtPhysicalMapParams{};

			Math::Vector2 terrainMeterSize;
			float terrainHeightScale;
			uint32_t culledPatchListIndex;

			uint32_t lodDebug;
			uint32_t pageTableMapIndex;
			uint32_t physicalAlbedoMapIndex;
			uint32_t physicalNormalMapIndex;

			uint32_t textureArrayIndex;
			float pad1;
			float pad2;
			float pad3;
		};

	public:
		TerrainPipelinePass(TerrainRenderer* renderer);
		~TerrainPipelinePass();

		void Initialize();

		void AddPass();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		TerrainBuilderPassData mTerrainBuilderPassData;
		TerrainRendererPassData mTerrainRendererPassData;

		TextureWrap mTerrainFeedbackMap;
		struct QueuedReadbackFeedback {
		public:
			uint64_t renderFrameFenceValue{ 0u };	// �ñ�������Ⱦ���߳�д�� RVT�߳�ֻ��
			std::atomic<bool> isFresh{ false };		// �ñ�������Ⱦ���߳����� RVT�߳̽��з������޸�
		};
		std::vector<QueuedReadbackFeedback> mQueuedReadbacks;
		std::vector<BufferWrap> mTerrainReadbackBuffers;
	};

}