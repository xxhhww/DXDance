#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/ClusterTree.h"
#include "Renderer/Model.h"
#include <vector>
#include <string>

namespace Renderer {

	class RenderEngine;

	struct TempInstanceData {
	public:
		Math::Vector3 position;
		Math::Vector4 quaternion;
		Math::Vector3 scaling;
	};

	class HierarchyInstancedStaticMesh {
	public:
		HierarchyInstancedStaticMesh(RenderEngine* renderEngine, const std::string& instanceName, const std::string& instancePath);
		~HierarchyInstancedStaticMesh() = default;

		/*
		* 构建
		*/
		void BuildTree();

		/*
		* 剔除
		*/
		void Cull(const Math::Vector3& cameraPosition, std::vector<int32_t>& clusterNodeIndex);

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
		TextureWrap mAlbedoMap;
		TextureWrap mNormalMap;
		TextureWrap mRoughnessMap;
	};

}