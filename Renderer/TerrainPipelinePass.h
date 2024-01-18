#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/TerrainRenderer.h"

#include "Renderer/Mesh.h"

namespace Renderer {

	class TerrainRenderer;

	class TerrainPipelinePass {
	public:
		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC;		// 用户控制的节点评估系数

			Math::Vector2 terrainMeterSize;		// 世界在XZ轴方向的大小(米)
			float         terrainHeightScale;	// Y轴缩放大小
			uint32_t      useFrustumCull;

			uint32_t currPassLOD;
			uint32_t consumeNodeListIndex;
			uint32_t appendNodeListIndex;
			uint32_t finalNodeListIndex;

			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;
			uint32_t culledPatchListIndex;
			float    pad;
		};

		struct TerrainRendererPassData {
			Math::Vector2 terrainMeterSize;
			float         terrainHeightScale;
			uint32_t      culledPatchListIndex;

			uint32_t terrainTextureArrayIndex;
			uint32_t terrainHeightMapAtlasIndex;
			uint32_t terrainAlbedoMapAtlasIndex;
			uint32_t terrainNormalMapAtlasIndex;

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

			uint32_t pageTableMapIndex;
			uint32_t physicalAlbedoMapIndex;
			uint32_t physicalNormalMapIndex;
			float    pad1;

			uint32_t lodDebug;
			float    pad2;
			float    pad3;
			float    pad4;
		};

	public:
		TerrainPipelinePass(TerrainRenderer* renderer);
		~TerrainPipelinePass();

		void AddPass();

	private:
		void CreateGraphicsObject();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		TerrainSetting& mTerrainSetting;

		struct RenderPatch {
			Math::Vector2 position;
			Math::Vector2 minmaxHeight;
			uint32_t lod;
			float pad1;
			float pad2;
			float pad3;
		};
		struct NodeLocation {
			uint32_t x;
			uint32_t y;
		};
		std::vector<NodeLocation> mMaxLodNodeList;

		BufferWrap mPatchMeshVertexBuffer;
		BufferWrap mPatchMeshIndexBuffer;
		uint32_t   mPatchMeshIndexCount;

		TerrainBuilderPassData mTerrainBuilderPassData;
		TerrainRendererPassData mTerrainRendererPassData;

		TextureWrap mTerrainFeedbackMap;
		struct QueuedReadbackFeedback {
		public:
			uint64_t renderFrameFenceValue{ 0u };	// 该变量由渲染主线程写入 RVT线程只读
			std::atomic<bool> isFresh{ false };		// 该变量由渲染主线程与入 RVT线程进行访问与修改
		};
		std::vector<QueuedReadbackFeedback> mQueuedReadbacks;
		std::vector<BufferWrap> mTerrainReadbackBuffers;
	};

}