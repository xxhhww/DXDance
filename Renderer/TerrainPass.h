#pragma once
#include <DirectStorage/dstorage.h>
#include "RenderGraph.h"

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
			Math::Vector2 worldSize{ 10240u, 10240u };					// 世界在XZ轴方向的大小(米)
			uint32_t heightScale = 2048u;
			uint32_t currPassLOD;
			uint32_t consumeNodeListIndex;
			uint32_t appendNodeListIndex;
			uint32_t finalNodeListIndex;
			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;
			uint32_t culledPatchListIndex;
			uint32_t minmaxHeightMapIndex;
			uint32_t useFrustumCull = 1u;
		};

		struct TerrainRendererPassData {
			Math::Vector2 worldSize{ 10240u, 10240u };
			uint32_t heightScale = 2048u;
			uint32_t culledPatchListIndex;
			uint32_t heightMapIndex;
			uint32_t diffuseMapIndex;
			uint32_t normalMapIndex;
			float pad3;
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
		uint32_t maxLOD{ 5u };	// 最大LOD等级
		uint32_t mostDetailNodeSize{ 64u }; // 最精细的节点的大小(单位: 米)
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LODDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		TerrainBuilderPassData  terrainBuilderPassData;
		TerrainRendererPassData terrainRendererPassData;

		std::unique_ptr<Renderer::Mesh> patchMesh;
		std::unique_ptr<Renderer::Texture> minmaxHeightMap;
		std::unique_ptr<Renderer::Texture> albedoMap;
		std::unique_ptr<Renderer::Texture> normalMap;
		std::unique_ptr<Renderer::Texture> heightMap;

	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
		/*
		* 更新节点和LOD的描述数组
		*/
		void UpdateNodeAndLodDescriptorArray();
	};
}