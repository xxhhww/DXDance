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

		inline int32_t GetClusterNodeSize() const { return mClusterTree->clusterNodes.size(); }

	private:
		void Initialize();

	private:
		inline static int32_t smLodGroupSize = 3u;

		RenderEngine* mRenderEngine = nullptr;
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

		BufferWrap mGpuTransformsBuffer;
		BufferWrap mGpuClusterNodesBuffer;
		BufferWrap mGpuSortedInstancesBuffer;

		// BufferWrap mCpuCulledClusterNodesIndexBuffer;						// Cpu剔除后的可见的ClusterNodes索引(叶节点索引)
		BufferWrap mGpuCulledVisibleClusterNodesIndexBuffer;				// Gpu剔除后的可见的ClusterNodes索引(叶节点索引)
		std::vector<BufferWrap> mGpuCulledVisibleLodInstanceIndexBuffers;	// 可见的实例索引(分LOD)
	};

}