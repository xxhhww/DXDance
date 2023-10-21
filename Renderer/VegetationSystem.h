#pragma once
#include "Renderer/VegetationDataCache.h"

namespace Renderer {

	/*
	* ��Ⱥ
	*/
	struct TreeCluster {
	public:
		Math::Vector4 treeClusterRect;
	};

	/*
	* ��Ⱥ
	*/
	struct GrassCluster {
	public:
		int32_t grassBladeBufferIndex{ -1 };	// �ݵ㿪ʼλ�õ�����
		Math::Vector4 grassClusterRect;
	};

	class VegetationSystem {
	public:
		void Initialize(RenderEngine* renderEngine);

		/*
		* �߼����£��������µ�GrassCluster���룬�ɵ�GrassCluster����
		*/
		void Update();

		void AddPass(RenderGraph& renderGraph);

		/*
		* ����ǰλ�ù̶�������ڵ�
		*/
		const Math::Vector2& GetFixedPosition(const Math::Vector2& currPosition, const Math::Vector2& gridSize);

	private:
		Math::Vector2 mLastFixedPositionForGrass;
		Math::Vector4 mGrassClusterRect;	// ��Ⱥ�ľ��η�Χ

		Math::Vector2 mLastFixedPositionForTree;
		Math::Vector4 mTreeClusterRect;		// ��Ⱥ�ľ��η�Χ
	};

}