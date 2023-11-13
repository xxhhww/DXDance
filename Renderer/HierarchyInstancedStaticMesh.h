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
		HierarchyInstancedStaticMesh(RenderEngine* renderEngine, const std::string& instanceName, const std::string& instancePath);
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
		* 获取Lod个数
		*/
		inline int32_t GetLodGroupSize() const { return smLodGroupSize; }

		/*
		* 获取LodGroups
		*/
		inline auto& GetLodGroups() { return mLodGroups; }

	private:
		void Initialize();

	private:
		inline static int32_t smLodGroupSize = 3u;

		int32_t mInstanceCountPerCluster{ 512 };	// 集群节点内部的实例化个数

		RenderEngine* mRenderEngine{ nullptr };
		std::string mInstanceName;
		std::string mInstancePath;

		std::unique_ptr<ClusterTree> mClusterTree;

		std::vector<std::unique_ptr<Renderer::Model>> mLodGroups;
		Math::BoundingBox mInstanceBoundingBox;	// 取自LOD0
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

		// BufferWrap mCpuCulledClusterNodesIndexBuffer;						// Cpu剔除后的可见的ClusterNodes索引(叶节点索引)
		BufferWrap mGpuCulledVisibleClusterNodesIndexBuffer;				// Gpu剔除后的可见的ClusterNodes索引(叶节点索引)
		std::vector<BufferWrap> mGpuCulledVisibleLodInstanceIndexBuffers;	// 可见的实例索引(分LOD)
	};

}