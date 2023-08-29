#pragma once
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class RenderEngine;
	class RingFrameTracker;
	class RVTUpdater;

	class TerrainSystem {
	public:
		struct NodeDescriptor {
			uint32_t isBranch = true;
			float pad1;
			float pad2;
			float pad3;
		};

		struct LODDescriptor {
			uint32_t nodeSize;			// 该LOD中每一个Node的边长(米)(Node是正方形)
			uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
			uint32_t nodeCount;			// 该LOD中的Node的总个数
			float pad1;
		};

		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC{ 1.2f, 0.0f, 0.0f, 0.0f };	// 用户控制的节点评估系数

			Math::Vector2 worldMeterSize{ 5120u, 5120u };				// 世界在XZ轴方向的大小(米)
			uint32_t heightScale{ 4096u };
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
			Math::Vector4 vtFeedbackParams;
			Math::Vector4 vtRealRect;

			Math::Vector2 worldMeterSize{ 5120u, 5120u };
			uint32_t heightScale{ 4096u };
			uint32_t culledPatchListIndex;

			uint32_t heightMapIndex;
			uint32_t normalMapIndex;
			uint32_t lodDebug{ 0u };
			uint32_t splatMapIndex;

			uint32_t rChannelAlbedoMapIndex;
			uint32_t rChannelNormalMapIndex;
			uint32_t rChannelRoughnessMapIndex;
			uint32_t rChannelHeightMapIndex;

			uint32_t gChannelAlbedoMapIndex;
			uint32_t gChannelNormalMapIndex;
			uint32_t gChannelRoughnessMapIndex;
			uint32_t gChannelHeightMapIndex;

			uint32_t bChannelAlbedoMapIndex;
			uint32_t bChannelNormalMapIndex;
			uint32_t bChannelRoughnessMapIndex;
			uint32_t bChannelHeightMapIndex;

			uint32_t aChannelAlbedoMapIndex;
			uint32_t aChannelNormalMapIndex;
			uint32_t aChannelRoughnessMapIndex;
			uint32_t aChannelHeightMapIndex;
		};

		struct NodeLocation {
			uint32_t x;
			uint32_t y;
		};

		struct RenderPatch {
			Math::Vector2 position;
			Math::Vector2 minMaxHeight;
			uint32_t lod;
			float pad1;
			float pad2;
			float pad3;
		};

	public:
		bool isInitialized{ false };
		Math::Vector2 worldMeterSize{ 5120u, 5120u };
		float worldHeightScale{ 4096u };
		uint32_t maxLOD{ 4u };	// 最大LOD等级
		uint32_t mostDetailNodeMeterSize{ 64u }; // 最精细的节点的大小(单位: 米)
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LODDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		TerrainBuilderPassData  terrainBuilderPassData;
		TerrainRendererPassData terrainRendererPassData;

		std::unique_ptr<Renderer::Mesh> patchMesh;
		TextureWrap minmaxHeightMap;
		TextureWrap heightMap;
		TextureWrap normalMap;

		TextureWrap splatMap;	// 纹理混合

		// channel R
		TextureWrap grassAlbedoMap;
		TextureWrap grassNormalMap;
		TextureWrap grassRoughnessMap;
		TextureWrap grassHeightMap;

		// channel G
		TextureWrap mudAlbedoMap;
		TextureWrap mudNormalMap;
		TextureWrap mudRoughnessMap;
		TextureWrap mudHeightMap;

		// channel B
		TextureWrap cliffAlbedoMap;
		TextureWrap cliffNormalMap;
		TextureWrap cliffRoughnessMap;
		TextureWrap cliffHeightMap;

		// channel A
		TextureWrap snowAlbedoMap;
		TextureWrap snowNormalMap;
		TextureWrap snowRoughness;
		TextureWrap snowHeightMap;

	public:
		TerrainSystem();
		~TerrainSystem();

		/*
		* 地形初始化
		*/
		void Initialize(RenderEngine* renderEngine);

		/*
		* 添加TerrainPass至RenderGraph
		*/
		void AddPass(RenderEngine* renderEngine);

		/*
		* 渲染帧完成后的回调函数
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

	private:
		void UpdateNodeAndLodDescriptorArray();

	private:
		TextureWrap mTerrainFeedbackMap;
		RVTUpdater* mRVTUpdater{ nullptr };
	};

}