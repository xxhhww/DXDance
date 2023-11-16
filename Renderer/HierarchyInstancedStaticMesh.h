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
	* 将实例化数据从Houdini坐标系转到当前坐标系
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
			float distanceCullStartDistance,	// 启用DistanceCull的开始距离
			float distanceCullFactor,			// 控制DistanceCull的剔除幅度(取值为 0.0f - 1.0f)，值越大剔除越多
			int32_t instanceLodGroupSize,
			const std::vector<float>& instanceLodDistancesScale);

		~HierarchyInstancedStaticMesh() = default;

		/*
		* 构建
		*/
		void BuildTree();

		/*
		* 集群节点个数
		*/
		inline int32_t GetClusterNodeSize() const { return mClusterTree->clusterNodes.size(); }

		/*
		* 集群节点内部的实例个数
		*/
		inline int32_t GetInstanceCountPerCluster() const { return mInstanceCountPerCluster; }

		/*
		* 获取可视化距离
		*/
		inline float GetInstanceVisibleDistance() const { return mInstanceVisibleDistance; }

		/*
		* 获取DistanceCull的开始距离
		*/
		inline float GetDistanceCullStartDistance() const { return mDistanceCullStartDistance; }

		/*
		* 获取DistanceCull的剔除参数
		*/
		inline float GetDistanceCullFactor() const { return mDistanceCullFactor; }


		/*
		* 获取总的实例个数
		*/
		inline int32_t GetTotalInstanceCount() const { return mTransforms.size(); }

		/*
		* 获取Lod个数
		*/
		inline int32_t GetLodGroupSize() const { return mLodGroupSize; }

		/*
		* 获取Lod距离数组
		*/
		inline const auto& GetLodDistances() const { return mLodDistances; }

		/*
		* 获取LodGroups
		*/
		inline auto& GetLodGroups() { return mLodGroups; }

	private:
		void Initialize();

	private:
		int32_t mInstanceCountPerCluster;	// 集群节点内部的实例化个数
		float mInstanceVisibleDistance;		// 实例的可视化距离
		float mDistanceCullStartDistance;	// DistanceCull的开始距离
		float mDistanceCullFactor;			// DistanceCull的剔除幅度(取值为 0.0f - 1.0f)，值越大剔除越多

		int32_t mLodGroupSize;
		std::vector<float> mLodDistancesScale;	// 各个Lod的终点距离的缩放
		std::vector<float> mLodDistances;		// 各个Lod的终点距离

		RenderEngine* mRenderEngine{ nullptr };
		std::string mInstanceName;
		std::string mInstancePath;

		std::unique_ptr<ClusterTree> mClusterTree;

		std::vector<std::unique_ptr<Renderer::Model>> mLodGroups;
		Math::BoundingBox mInstanceBoundingBox;	// 取自LOD0
		std::vector<Math::Matrix4> mTransforms;	// Matrix已被转置
		std::vector<Math::BoundingBox> mTransformedBoundingBoxs;

		TextureWrap mAlbedoMap;
		TextureWrap mNormalMap;
		TextureWrap mRoughnessMap;
		TextureWrap mAoMap;

		BufferWrap mGpuTransformsBuffer;
		BufferWrap mGpuClusterNodesBuffer;
		BufferWrap mGpuSortedInstancesBuffer;
		BufferWrap mGpuTransformedBoundingBoxBuffer;

		BufferWrap mGpuCulledVisibleClusterNodesIndexBuffer;				// Gpu剔除后的可见的ClusterNodes索引(叶节点索引)
		std::vector<BufferWrap> mGpuCulledVisibleInstanceIndexBuffers;		// 可见的实例索引(分LOD)
	};

}