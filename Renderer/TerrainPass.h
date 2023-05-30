#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class TerrainPass {
	public:
		struct NodeDescriptor {
			uint32_t isBranch;
			float pad1;
			float pad2;
			float pad3;
		};

		struct LODDescriptor {
			float nodeSize;         // ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
			float nodeStartOffset;  // ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
			float pad1;
			float pad2;
		};

		struct PassData {
			Math::Vector3 nodeEvaluationC{ 1.2f, 0.0f, 0.0f };	// �û����ƵĽڵ�����ϵ��
			float pad1;
			Math::Vector2 worldSize{ 10240u, 10240u };			// ������XZ�᷽��Ĵ�С(��)
			uint32_t currPassLOD;
			uint32_t currLODNodeListIndex;
			uint32_t nextLODNodeListIndex;
			uint32_t finalNodeListIndex;
			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;
		};

		struct NodeLocation {
			uint32_t x;
			uint32_t y;
		};

	public:
		uint32_t maxLOD{ 5u };	// ���LOD�ȼ�
		uint32_t mostDetailNodeSize{ 64u }; // �ϸ�Ľڵ�Ĵ�С(��λ: ��)
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LODDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		PassData passData;

		bool isInitialized{ false }; // �Ƿ���Ҫ����Node��Lod��DescriptorArray

	public:
		void AddPass(RenderGraph& renderGraph);

	private:
		void UpdateNodeAndLodDescriptorArray();
	};

}