#pragma once
#include "Renderer/HierarchyInstancedStaticMesh.h"


namespace Renderer {

	class RenderEngine;

	class DetailObjectSystem {
	public:
		struct CullClusterPassData {
			uint32_t clusterNodeBufferIndex;
			uint32_t visibleClusterNodeIndexBufferIndex;
			uint32_t clusterNodeBufferSize;
			float    instanceVisibleDistance;				// ���ӻ�����
		};

		struct CullStaticInstancePassData {
			uint32_t clusterNodeBufferIndex;
			uint32_t transformedBoundingBoxBufferIndex;
			uint32_t sortedInstancesBufferIndex;
			uint32_t visibleClusterNodeIndexBufferIndex;
			
			uint32_t visibleLod0InstanceIndexBufferIndex;
			uint32_t visibleLod1InstanceIndexBufferIndex;
			uint32_t visibleLod2InstanceIndexBufferIndex;
			uint32_t instanceCountPerCluster;

			float lod0InstanceVisibleDistance;
			float lod1InstanceVisibleDistance;
			float lod2InstanceVisibleDistance;
			float instanceVisibleDistance;

			uint32_t totalInstanceCount;
			float pad1;
			float pad2;
			float pad3;
		};

		// Per Lod Per HierarchyInstancedStaticMesh
		struct RenderStaticInstancePassData {
			// ʵ����������
			uint32_t instanceAlbedoMapIndex;
			uint32_t instanceNormalMapIndex;
			uint32_t instanceRoughnessMapIndex;
			uint32_t instanceAoMapIndex;

			uint32_t transformsBufferIndex;
			uint32_t visibleInstanceIndexBufferIndex;	// �����ŵ���Instance��ֱ������
			float pad1;
			float pad2;
		};

	public:
		DetailObjectSystem(RenderEngine* renderEngine);
		~DetailObjectSystem() = default;

		/*
		* ��ʼ��
		*/
		void Initialize(RenderEngine* renderEngine);
		
		/*
		* ���Pass
		*/
		void AddPass(RenderEngine* renderEngine);

	private:
		inline static uint32_t smCullClusterNodeThreadSizeInGroup = 8u;
		inline static uint32_t smCullStaticInstanceThreadSizeInGroup = 8u;

		RenderEngine* mRenderEngine{ nullptr };

		int32_t mHierarchyInstancedStaticMeshSize{ 0 };
		int32_t mLodCount{ 0 };	// ���е�Lod����

		std::vector<std::unique_ptr<HierarchyInstancedStaticMesh>> mHierarchyInstancedStaticMeshs;
		std::vector<CullClusterPassData> mCullClusterPassDatas;
		std::vector<CullStaticInstancePassData> mCullStaticInstancePassDatas;
		std::vector<RenderStaticInstancePassData> mRenderStaticInstancePassDatas;
	};

}