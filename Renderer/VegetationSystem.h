#pragma once
#include "Renderer/VegetationDataCache.h"

namespace Renderer {

	/*
	* 树群
	*/
	struct TreeCluster {
	public:
		Math::Vector4 treeClusterRect;
	};

	/*
	* 草群
	*/
	struct GrassCluster {
	public:
		int32_t grassBladeBufferIndex{ -1 };	// 草点开始位置的索引
		Math::Vector4 grassClusterRect;
	};

	class VegetationSystem {
	public:
		void Initialize(RenderEngine* renderEngine);

		/*
		* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出
		*/
		void Update();

		void AddPass(RenderGraph& renderGraph);

		/*
		* 将当前位置固定到网格节点
		*/
		const Math::Vector2& GetFixedPosition(const Math::Vector2& currPosition, const Math::Vector2& gridSize);

	private:
		Math::Vector2 mLastFixedPositionForGrass;
		Math::Vector4 mGrassClusterRect;	// 草群的矩形范围

		Math::Vector2 mLastFixedPositionForTree;
		Math::Vector4 mTreeClusterRect;		// 树群的矩形范围
	};

}