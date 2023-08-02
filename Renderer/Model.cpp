#include "Model.h"

#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	Model::Model(
		const GHL::Device* device,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		const std::string& path) 
	: mDevice(device) 
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator)
	, mPath(path) {}

	void Model::LoadDataFromDisk(IDStorageQueue* copyDsQueue, GHL::Fence* copyFence) {
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(mPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			ASSERT_FORMAT(false, "Assimp Import Failed");
		}

		aiMatrix4x4 identity;

		ProcessNode(&identity, scene->mRootNode, scene, mMeshs, copyDsQueue, copyFence);
	}

	void Model::ProcessMaterials(const struct aiScene* p_scene, std::vector<std::string>& p_materials) {
		return;
	}

	void Model::ProcessNode(void* p_transform, struct aiNode* p_node, const struct aiScene* p_scene, std::vector<std::unique_ptr<Mesh>>& p_meshes, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence) {
		aiMatrix4x4 nodeTransformation = *reinterpret_cast<aiMatrix4x4*>(p_transform) * p_node->mTransformation;

		// Process all the node's meshes (if any)
		for (uint32_t i = 0; i < p_node->mNumMeshes; ++i) {
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			aiMesh* mesh = p_scene->mMeshes[p_node->mMeshes[i]];
			ProcessMesh(&nodeTransformation, mesh, p_scene, vertices, indices);

			BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Vertex);
			vbDesc.size = sizeof(Vertex) * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = sizeof(uint32_t) * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			p_meshes.emplace_back(std::make_unique<Mesh>(
				mDevice,
				ResourceFormat{ mDevice, vbDesc },
				ResourceFormat{ mDevice, ibDesc },
				mDescriptorAllocator, 
				mHeapAllocator));
			p_meshes.back()->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
		}

		// Then do the same for each of its children
		for (uint32_t i = 0; i < p_node->mNumChildren; ++i) {
			ProcessNode(&nodeTransformation, p_node->mChildren[i], p_scene, p_meshes, copyDsQueue, copyFence);
		}
	}

	void Model::ProcessMesh(void* p_transform, struct aiMesh* p_mesh, const struct aiScene* p_scene, std::vector<Vertex>& p_outVertices, std::vector<uint32_t>& p_outIndices) {
		aiMatrix4x4 meshTransformation = *reinterpret_cast<aiMatrix4x4*>(p_transform);

		for (uint32_t i = 0; i < p_mesh->mNumVertices; ++i) {
			aiVector3D position = meshTransformation * p_mesh->mVertices[i];
			aiVector3D normal = meshTransformation * (p_mesh->mNormals ? p_mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f));
			aiVector3D texCoords = p_mesh->mTextureCoords[0] ? p_mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);
			aiVector3D tangent = p_mesh->mTangents ? meshTransformation * p_mesh->mTangents[i] : aiVector3D(0.0f, 0.0f, 0.0f);
			aiVector3D bitangent = p_mesh->mBitangents ? meshTransformation * p_mesh->mBitangents[i] : aiVector3D(0.0f, 0.0f, 0.0f);
			aiColor4D  color = p_mesh->mColors[0] ? p_mesh->mColors[0][i] : aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);

			p_outVertices.push_back (
				{
					position.x,
					position.y,
					position.z,
					texCoords.x,
					texCoords.y,
					normal.x,
					normal.y,
					normal.z,
					tangent.x,
					tangent.y,
					tangent.z,
					bitangent.x,
					bitangent.y,
					bitangent.z,
					color.r,
					color.g,
					color.b,
					color.a
				}
			);
		}

		for (uint32_t faceID = 0; faceID < p_mesh->mNumFaces; ++faceID) {
			auto& face = p_mesh->mFaces[faceID];

			for (size_t indexID = 0; indexID < 3; ++indexID) {
				p_outIndices.push_back(face.mIndices[indexID]);
			}
		}
	}

}