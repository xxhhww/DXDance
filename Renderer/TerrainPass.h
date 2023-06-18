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
			uint32_t nodeSize;			// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
			uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
			uint32_t nodeCount;			// ��LOD�е�Node���ܸ���
			float pad1;
		};

		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC{ 1.2f, 0.0f, 0.0f, 0.0f };	// �û����ƵĽڵ�����ϵ��
			Math::Vector2 worldSize{ 10240u, 10240u };					// ������XZ�᷽��Ĵ�С(��)
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
		uint32_t maxLOD{ 5u };	// ���LOD�ȼ�
		uint32_t mostDetailNodeSize{ 64u }; // �ϸ�Ľڵ�Ĵ�С(��λ: ��)
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
		* ���½ڵ��LOD����������
		*/
		void UpdateNodeAndLodDescriptorArray();
	};
}