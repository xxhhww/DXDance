#pragma once
#include "RenderGraph.h"

namespace Renderer {

	class TerrainPass {
	public:
		struct NodeDescriptor {
			uint32_t isBranch = true;
			float pad1;
			float pad2;
			float pad3;
		};

		struct LodDescriptor {
			uint32_t nodeSize;			// 该LOD中每一个Node的边长(米)(Node是正方形)
			uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
			uint32_t nodeCount;			// 该LOD中的Node的总个数
			float pad2;
		};

		struct PassData {
			Math::Vector3 nodeEvaluationC{ 1.2f, 0.0f, 0.0f };	// 用户控制的节点评估系数
			float pad1;
			Math::Vector2 worldSize{ 10240u, 10240u };			// 世界在XZ轴方向的大小(米)
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
		uint32_t maxLOD{ 5u };	// 最大LOD等级
		uint32_t mostDetailNodeSize{ 64u }; // 最精细的节点的大小(单位: 米)
		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LodDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		PassData passData;

		bool isInitialized{ false }; // 是否需要更新Node与Lod的DescriptorArray

	public:
		void AddPass(RenderGraph& renderGraph);

	private:
		void UpdateNodeAndLodDescriptorArray();
	};

}