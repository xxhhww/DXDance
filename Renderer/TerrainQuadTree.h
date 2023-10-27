#pragma once
#include "Renderer/VegetationSystem.h"

namespace Renderer {

	/*
	* 四叉树节点内容描述
	*/
	struct TerrainQuadNodeDescriptor {
	public:
		uint8_t nodeLocationX;
		uint8_t nodeLocationY;
		uint8_t nodeLOD;
		uint8_t isLoaded;		// 四叉树节点对应的大地形纹理块是否被加载(地形渲染时如果该纹理块未被加载则取其父节点)
	};

	/*
	* 四叉树节点
	*/
	class TerrainQuadNode {
	public:
		uint32_t descriptorIndex{ 0u };	// 内容描述索引(索引至全节点内容描述列表)

		std::vector<TerrainQuadNode*> mChildQuadNodes;
	};

	/*
	* 地形四叉树
	*/
	class TerrainQuadTree {
	public:
		void Initialize(RenderEngine* renderEngine);

		void AddPass(RenderEngine* renderEngine);

	private:
		inline static float smWorldMeterSize{ 8192.0f };
		inline static float smWorldHeightScale{ 1325.0f };
		inline static float smMostDetailNodeMeterSize{ 64.0f };
		inline static float smLeastDetailNodeMeterSize{ 1024.0f };
		inline static uint32_t smMaxLOD{ 4u };	// pow(2, 4) * 64 = 1024.0f

		std::vector<TerrainQuadTree*> mRootQuadNodes;	// 四叉树的n个根节点
		std::vector<TerrainQuadNodeDescriptor> mTerrainQuadNodeDescriptors;	// 四叉树全节点内容描述列表
	};





}