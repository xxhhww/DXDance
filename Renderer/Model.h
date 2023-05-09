#pragma once
#include "Mesh.h"

#include <DirectStorage/dstorage.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

namespace GHL {
	class Fence;
}

namespace Renderer {

	class Model {
	public:
		Model(
			const GHL::Device* device,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator,
			const std::string& path);

		~Model() = default;

		void LoadDataFromDisk(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence);

	private:
		void ProcessMaterials(const aiScene* p_scene, std::vector<std::string>& p_materials);;
		void ProcessNode(void* p_transform, aiNode* p_node, const aiScene* p_scene, std::vector<std::unique_ptr<Mesh>>& p_meshes, IDStorageQueue* copyDsQueue);
		void ProcessMesh(void* p_transform, aiMesh* p_mesh, const aiScene* p_scene, std::vector<Vertex>& p_outVertices, std::vector<uint32_t>& p_outIndices);

	private:
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };

		std::string mPath;

		std::vector<std::unique_ptr<Mesh>> mMeshs;

		DirectX::BoundingBox mBoundingBox; // АќЮЇКа
	};

}