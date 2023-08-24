#pragma once
#include <DirectStorage/dstorage.h>
#include <vector>
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"

namespace GHL {
	class Device;
	class Fence;
}

namespace Renderer {
	class Mesh;
	class RenderEngine;

	class TerrainPass {
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
			uint32_t useFrustumCull{ 1u };

			uint32_t nearbyNodeListIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct TerrainRendererPassData {
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
			uint32_t rChannelDisplacementMapIndex;
			
			uint32_t gChannelAlbedoMapIndex;
			uint32_t gChannelNormalMapIndex;
			uint32_t gChannelRoughnessMapIndex;
			uint32_t gChannelDisplacementMapIndex;

			uint32_t bChannelAlbedoMapIndex;
			uint32_t bChannelNormalMapIndex;
			uint32_t bChannelRoughnessMapIndex;
			uint32_t bChannelDisplacementMapIndex;

			uint32_t aChannelAlbedoMapIndex;
			uint32_t aChannelNormalMapIndex;
			uint32_t aChannelRoughnessMapIndex;
			uint32_t aChannelDisplacementMapIndex;
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

		// channel G
		TextureWrap mudAlbedoMap;
		TextureWrap mudNormalMap;
		TextureWrap mudRoughnessMap;

		// channel B
		TextureWrap cliffAlbedoMap;
		TextureWrap cliffNormalMap;
		TextureWrap cliffRoughnessMap;

		// channel A
		TextureWrap snowAlbedoMap;
		TextureWrap snowNormalMap;
		TextureWrap snowRoughness;

	public:
		void AddPreDepthPass(RenderGraph& renderGraph);

		void AddShadowPass(RenderGraph& renderGraph);

		void AddPass(RenderGraph& renderGraph);

		void AddForwardPlusPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
		/*
		* 更新节点和LOD的描述数组
		*/
		void UpdateNodeAndLodDescriptorArray();
	};
}