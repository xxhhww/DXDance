#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/ClusterTree.h"
#include "Renderer/Model.h"
#include <vector>
#include <string>

namespace Renderer {

	class RenderEngine;
	class DetailObjectSystem;
	struct TempInstanceData {
	public:
		Math::Vector3 position;
		Math::Vector4 quaternion;
		Math::Vector3 scaling;
	};

	class HierarchyInstancedStaticMesh {
		friend class DetailObjectSystem;
	public:
		HierarchyInstancedStaticMesh(
			RenderEngine* renderEngine, 
			const std::string& instanceName, 
			const std::string& instancePath,
			int32_t instanceCountPerCluster,
			int32_t instanceVisibleDistance);

		~HierarchyInstancedStaticMesh() = default;

		/*
		* ����
		*/
		void BuildTree();

		/*
		* ��Ⱥ�ڵ����
		*/
		inline int32_t GetClusterNodeSize() const { return mClusterTree->clusterNodes.size(); }

		/*
		* ��Ⱥ�ڵ��ڲ���ʵ������
		*/
		inline int32_t GetInstanceCountPerCluster() const { return mInstanceCountPerCluster; }

		/*
		* ��ȡ���ӻ�����
		*/
		inline float GetInstanceVisibleDistance() const { return mInstanceVisibleDistance; }

		/*
		* ��ȡ�ܵ�ʵ������
		*/
		inline int32_t GetTotalInstanceCount() const { return mTransforms.size(); }

		/*
		* ��ȡLod����
		*/
		inline int32_t GetLodGroupSize() const { return smLodGroupSize; }

		/*
		* ��ȡLod��������
		*/
		inline const auto& GetLodDistances() const { return mLodDistances; }

		/*
		* ��ȡLodGroups
		*/
		inline auto& GetLodGroups() { return mLodGroups; }

	private:
		void Initialize();

	private:
		inline static int32_t smLodGroupSize = 3u;

		int32_t mInstanceCountPerCluster{ 512 };	// ��Ⱥ�ڵ��ڲ���ʵ��������
		float mInstanceVisibleDistance{ 4096.0f };	// ʵ���Ŀ��ӻ�����
		std::vector<float> mLodDistances;			// ����Lod���յ����

		RenderEngine* mRenderEngine{ nullptr };
		std::string mInstanceName;
		std::string mInstancePath;

		std::unique_ptr<ClusterTree> mClusterTree;

		std::vector<std::unique_ptr<Renderer::Model>> mLodGroups;
		Math::BoundingBox mInstanceBoundingBox;	// ȡ��LOD0
		std::vector<Math::Matrix4> mTransforms;
		std::vector<Math::BoundingBox> mTransformedBoundingBoxs;

		TextureWrap mAlbedoMap;
		TextureWrap mNormalMap;
		TextureWrap mRoughnessMap;
		TextureWrap mAoMap;

		BufferWrap mGpuTransformsBuffer;
		BufferWrap mGpuClusterNodesBuffer;
		BufferWrap mGpuSortedInstancesBuffer;
		BufferWrap mGpuTransformedBoundingBoxBuffer;

		// BufferWrap mCpuCulledClusterNodesIndexBuffer;						// Cpu�޳���Ŀɼ���ClusterNodes����(Ҷ�ڵ�����)
		BufferWrap mGpuCulledVisibleClusterNodesIndexBuffer;				// Gpu�޳���Ŀɼ���ClusterNodes����(Ҷ�ڵ�����)
		std::vector<BufferWrap> mGpuCulledVisibleInstanceIndexBuffers;		// �ɼ���ʵ������(��LOD)
	};

}