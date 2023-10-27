#pragma once
#include "Renderer/VegetationSystem.h"

namespace Renderer {

	/*
	* �Ĳ����ڵ���������
	*/
	struct TerrainQuadNodeDescriptor {
	public:
		uint8_t nodeLocationX;
		uint8_t nodeLocationY;
		uint8_t nodeLOD;
		uint8_t isLoaded;		// �Ĳ����ڵ��Ӧ�Ĵ����������Ƿ񱻼���(������Ⱦʱ����������δ��������ȡ�丸�ڵ�)
	};

	/*
	* �Ĳ����ڵ�
	*/
	class TerrainQuadNode {
	public:
		uint32_t descriptorIndex{ 0u };	// ������������(������ȫ�ڵ����������б�)

		std::vector<TerrainQuadNode*> mChildQuadNodes;
	};

	/*
	* �����Ĳ���
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

		std::vector<TerrainQuadTree*> mRootQuadNodes;	// �Ĳ�����n�����ڵ�
		std::vector<TerrainQuadNodeDescriptor> mTerrainQuadNodeDescriptors;	// �Ĳ���ȫ�ڵ����������б�
	};





}