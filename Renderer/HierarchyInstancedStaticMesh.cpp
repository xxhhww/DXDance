#include "Renderer/HierarchyInstancedStaticMesh.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"

namespace Renderer {

	HierarchyInstancedStaticMesh::HierarchyInstancedStaticMesh(RenderEngine* renderEngine, const std::string& instanceName, const std::string& instancePath)
	: mRenderEngine(renderEngine)
	, mInstanceName(instanceName)
	, mInstancePath(instancePath) {}

	void HierarchyInstancedStaticMesh::BuildTree() {
		Renderer::ClusterBuilder treeBuilder(mTransforms, mInstanceBoundingBox);
		treeBuilder.BuildTree();

		mClusterTree = std::move(treeBuilder.result);
	}

	void HierarchyInstancedStaticMesh::Cull(const Math::Vector3& cameraPosition, std::vector<int32_t>& clusterNodeIndex) {

	}

	void HierarchyInstancedStaticMesh::Initialize() {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStorage = mRenderEngine->mPipelineResourceStorage;
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();
		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		// LodGroups
		for (int32_t index = 0; index < smLodGroupSize; index++) {
			std::string lodPath = mInstancePath + "/Lod" + std::to_string(index) + ".fbx";
			std::unique_ptr<Renderer::Model> lodModel = std::make_unique<Renderer::Model>(device, descriptorAllocator, nullptr, lodPath);
			lodModel->LoadDataFromDisk(copyDsQueue, copyFence);
			
			mLodGroups.emplace_back(std::move(lodModel));
		}

		auto boundingBoxDx = mLodGroups.at(0)->GetFirstMesh()->GetBoundingBox();
		Math::Vector3 center(boundingBoxDx.Center.x, boundingBoxDx.Center.y, boundingBoxDx.Center.z);
		Math::Vector3 extends(boundingBoxDx.Extents.x, boundingBoxDx.Extents.y, boundingBoxDx.Extents.z);
		mInstanceBoundingBox.minPosition = (center - extends);
		mInstanceBoundingBox.maxPosition = (center + extends);

		// Material Texture
		mAlbedoMap = FixedTextureHelper::LoadFromFile(
			device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
			mInstancePath + "/Albedo.png");
		resourceStateTracker->StartTracking(mAlbedoMap);
		resourceStorage->ImportResource(mInstanceName + "_Albedo", mAlbedoMap);

		mNormalMap = FixedTextureHelper::LoadFromFile(
			device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
			mInstancePath + "/Normal.png");
		resourceStateTracker->StartTracking(mNormalMap);
		resourceStorage->ImportResource(mInstanceName + "_Normal", mNormalMap);

		mRoughnessMap = FixedTextureHelper::LoadFromFile(
			device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
			mInstancePath + "/Roughness.png");
		resourceStateTracker->StartTracking(mRoughnessMap);
		resourceStorage->ImportResource(mInstanceName + "_Roughness", mRoughnessMap);

		// 读取实例化数据
		std::string instanceDataPath = mInstancePath + "/InstanceData.bin";
		std::ifstream instanceDataStream(instanceDataPath);
		Tool::InputMemoryStream inputStream(instanceDataStream);

		int32_t instanceCount = inputStream.Size() / sizeof(TempInstanceData);
		mTransforms.resize(instanceCount);
		for (int32_t i = 0; i < instanceCount; i++) {
			TempInstanceData tempInstanceData;

			inputStream.Read(tempInstanceData.position);
			inputStream.Read(tempInstanceData.quaternion);
			inputStream.Read(tempInstanceData.scaling);

			mTransforms[i] = Math::Matrix4(tempInstanceData.position, tempInstanceData.quaternion, tempInstanceData.scaling);
		}

		// 构建集群树
		BuildTree();
	}

}