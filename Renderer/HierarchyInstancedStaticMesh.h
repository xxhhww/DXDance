#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/ClusterTree.h"
#include "Renderer/Model.h"
#include <vector>
#include <string>

namespace Renderer {

	class RenderEngine;
	class DetailObjectSystem;
	
	/*
	* ��ʵ�������ݴ�Houdini����ϵת����ǰ����ϵ
	*/
	struct HoudiniInstanceData {
	public:
		Math::Vector3 position;
		Math::Vector4 quaternion;
		Math::Vector3 scaling;

	public:
		HoudiniInstanceData(
			const Math::Vector3& _position, 
			const Math::Vector4& _quaternion, 
			const Math::Vector3& _scaling);

		~HoudiniInstanceData() = default;
	};

	class HierarchyInstancedStaticMesh {
		friend class DetailObjectSystem;
	public:
		HierarchyInstancedStaticMesh(
			RenderEngine* renderEngine, 
			const std::string& instanceName, 
			const std::string& instancePath,
			int32_t instanceCountPerCluster,
			float instanceVisibleDistance,
			float distanceCullStartDistance,	// ����DistanceCull�Ŀ�ʼ����
			float distanceCullFactor,			// ����DistanceCull���޳�����(ȡֵΪ 0.0f - 1.0f)��ֵԽ���޳�Խ��
			int32_t instanceLodGroupSize,
			const std::vector<float>& instanceLodDistancesScale);

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
		* ��ȡDistanceCull�Ŀ�ʼ����
		*/
		inline float GetDistanceCullStartDistance() const { return mDistanceCullStartDistance; }

		/*
		* ��ȡDistanceCull���޳�����
		*/
		inline float GetDistanceCullFactor() const { return mDistanceCullFactor; }


		/*
		* ��ȡ�ܵ�ʵ������
		*/
		inline int32_t GetTotalInstanceCount() const { return mTransforms.size(); }

		/*
		* ��ȡLod����
		*/
		inline int32_t GetLodGroupSize() const { return mLodGroupSize; }

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
		int32_t mInstanceCountPerCluster;	// ��Ⱥ�ڵ��ڲ���ʵ��������
		float mInstanceVisibleDistance;		// ʵ���Ŀ��ӻ�����
		float mDistanceCullStartDistance;	// DistanceCull�Ŀ�ʼ����
		float mDistanceCullFactor;			// DistanceCull���޳�����(ȡֵΪ 0.0f - 1.0f)��ֵԽ���޳�Խ��

		int32_t mLodGroupSize;
		std::vector<float> mLodDistancesScale;	// ����Lod���յ���������
		std::vector<float> mLodDistances;		// ����Lod���յ����

		RenderEngine* mRenderEngine{ nullptr };
		std::string mInstanceName;
		std::string mInstancePath;

		std::unique_ptr<ClusterTree> mClusterTree;

		std::vector<std::unique_ptr<Renderer::Model>> mLodGroups;
		Math::BoundingBox mInstanceBoundingBox;	// ȡ��LOD0
		std::vector<Math::Matrix4> mTransforms;	// Matrix�ѱ�ת��
		std::vector<Math::BoundingBox> mTransformedBoundingBoxs;

		TextureWrap mAlbedoMap;
		TextureWrap mNormalMap;
		TextureWrap mRoughnessMap;
		TextureWrap mAoMap;

		BufferWrap mGpuTransformsBuffer;
		BufferWrap mGpuClusterNodesBuffer;
		BufferWrap mGpuSortedInstancesBuffer;
		BufferWrap mGpuTransformedBoundingBoxBuffer;

		BufferWrap mGpuCulledVisibleClusterNodesIndexBuffer;				// Gpu�޳���Ŀɼ���ClusterNodes����(Ҷ�ڵ�����)
		std::vector<BufferWrap> mGpuCulledVisibleInstanceIndexBuffers;		// �ɼ���ʵ������(��LOD)
	};

}