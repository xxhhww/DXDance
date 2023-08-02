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
			uint32_t nodeSize;			// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
			uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
			uint32_t nodeCount;			// ��LOD�е�Node���ܸ���
			float pad1;
		};

		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC{ 1.2f, 0.0f, 0.0f, 0.0f };	// �û����ƵĽڵ�����ϵ��
			Math::Vector2 worldMeterSize{ 5120u, 5120u };				// ������XZ�᷽��Ĵ�С(��)
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
		};

		struct TerrainRendererPassData {
			Math::Vector2 worldMeterSize{ 5120u, 5120u };
			uint32_t heightScale{ 4096u };
			uint32_t culledPatchListIndex;

			uint32_t heightMapIndex;
			uint32_t normalMapIndex;
			uint32_t lodDebug{ 0u };
			float pad1;

			uint32_t groundGrassAlbedoMapIndex;
			uint32_t groundGrassNormalMapIndex;
			uint32_t groundGrassRoughnessMapIndex;
			uint32_t groundGrassDisplacementMapIndex;
			
			uint32_t groundRockAlbedoMapIndex;
			uint32_t groundRockNormalMapIndex;
			uint32_t groundRockRoughnessMapIndex;
			uint32_t groundRockDisplacementMapIndex;

			uint32_t groundMudAlbedoMapIndex;
			uint32_t groundMudNormalMapIndex;
			uint32_t groundMudRoughnessMapIndex;
			uint32_t groundMudDisplacementMapIndex;
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
		uint32_t maxLOD{ 4u };	// ���LOD�ȼ�
		uint32_t mostDetailNodeMeterSize{ 64u }; // �ϸ�Ľڵ�Ĵ�С(��λ: ��)
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LODDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		TerrainBuilderPassData  terrainBuilderPassData;
		TerrainRendererPassData terrainRendererPassData;

		std::unique_ptr<Renderer::Mesh> patchMesh;
		TextureWrap minmaxHeightMap;
		TextureWrap heightMap;
		TextureWrap normalMap;

		TextureWrap groundGrassAlbedoMap;		// �ݵط�������ͼ
		TextureWrap groundGrassNormalMap;		// �ݵط�����ͼ
		TextureWrap groundGrassRoughnessMap;	// �ݵشֲڶ���ͼ

		TextureWrap groundRockAlbedoMap;		// ��ʯ��������ͼ
		TextureWrap groundRockNormalMap;		// ��ʯ������ͼ
		TextureWrap groundRockRoughnessMap;		// ��ʯ�ֲڶ���ͼ

		TextureWrap groundMudAlbedoMap;			// ������������ͼ
		TextureWrap groundMudNormalMap;			// ����������ͼ
		TextureWrap groundMudRoughnessMap;		// �����ֲڶ���ͼ

	public:
		void AddPreDepthPass(RenderGraph& renderGraph);

		void AddShadowPass(RenderGraph& renderGraph);

		void AddPass(RenderGraph& renderGraph);

		void AddForwardPlusPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
		/*
		* ���½ڵ��LOD����������
		*/
		void UpdateNodeAndLodDescriptorArray();
	};
}