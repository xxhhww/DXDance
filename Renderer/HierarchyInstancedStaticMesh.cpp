#include "Renderer/HierarchyInstancedStaticMesh.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"

namespace Renderer {

	HierarchyInstancedStaticMesh::HierarchyInstancedStaticMesh(
		RenderEngine* renderEngine,
		const std::string& instanceName,
		const std::string& instancePath,
		int32_t instanceCountPerCluster,
		int32_t instanceVisibleDistance)
		: mRenderEngine(renderEngine)
		, mInstanceName(instanceName)
		, mInstancePath(instancePath)
		, mInstanceCountPerCluster(instanceCountPerCluster)
		, mInstanceVisibleDistance(instanceVisibleDistance) {}

	void HierarchyInstancedStaticMesh::BuildTree() {
		Renderer::ClusterBuilder treeBuilder(mTransforms, mInstanceBoundingBox);
		treeBuilder.BuildTree();

		mClusterTree = std::move(treeBuilder.result);
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

		// LodDistances
		mLodDistances.resize(smLodGroupSize);
		mLodDistances.at(0) = mInstanceVisibleDistance * (1.0f / 8.0f);	// 前八分之一为Lod0
		mLodDistances.at(1) = mInstanceVisibleDistance * (1.0f / 2.0f);	// 前二分之一为Lod1
		mLodDistances.at(2) = mInstanceVisibleDistance * (1.0f / 1.0f);	// 前一分之一为Lod2

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

		mAoMap = FixedTextureHelper::LoadFromFile(
			device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
			mInstancePath + "/Ao.png");
		resourceStateTracker->StartTracking(mAoMap);
		resourceStorage->ImportResource(mInstanceName + "_Ao", mAoMap);

		// 读取实例化数据
		std::string instanceDataPath = mInstancePath + "/InstanceData.bin";
		std::ifstream instanceDataStream(instanceDataPath);
		Tool::InputMemoryStream inputStream(instanceDataStream);

		int32_t instanceCount = inputStream.Size() / sizeof(TempInstanceData);
		mTransforms.resize(instanceCount);
		mTransformedBoundingBoxs.resize(instanceCount);
		for (int32_t i = 0; i < instanceCount; i++) {
			TempInstanceData tempInstanceData;

			inputStream.Read(tempInstanceData.position);
			inputStream.Read(tempInstanceData.quaternion);
			inputStream.Read(tempInstanceData.scaling);

			mTransforms[i] = Math::Matrix4(tempInstanceData.position, tempInstanceData.quaternion, tempInstanceData.scaling);
			mTransformedBoundingBoxs[i] = mInstanceBoundingBox.transformBy(mTransforms[i]);
		}

		// 构建集群树
		BuildTree();

		// 创建并上传transforms、clusterTree.clusterNodes 和 clusterTree.sortedInstances至GPU

		// 创建部分
		std::string resourceName = mInstanceName + "_TransformsBuffer";
		BufferDesc _GpuTransformsBufferDesc{};
		_GpuTransformsBufferDesc.stride = sizeof(Math::Matrix4);
		_GpuTransformsBufferDesc.size = _GpuTransformsBufferDesc.stride * mTransforms.size();
		_GpuTransformsBufferDesc.usage = GHL::EResourceUsage::Default;
		_GpuTransformsBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GpuTransformsBufferDesc.initialState = GHL::EResourceState::Common;
		_GpuTransformsBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::CopyDestination;
		mGpuTransformsBuffer = resourceAllocator->Allocate(device, _GpuTransformsBufferDesc, descriptorAllocator, nullptr);
		mGpuTransformsBuffer->SetDebugName(resourceName);
		renderGraph->ImportResource(resourceName, mGpuTransformsBuffer);
		resourceStateTracker->StartTracking(mGpuTransformsBuffer);

		resourceName = mInstanceName + "_ClusterNodesBuffer";
		BufferDesc _GpuClusterNodesBufferDesc{};
		_GpuClusterNodesBufferDesc.stride = sizeof(Renderer::ClusterNode);
		_GpuClusterNodesBufferDesc.size = _GpuClusterNodesBufferDesc.stride * mClusterTree->clusterNodes.size();
		_GpuClusterNodesBufferDesc.usage = GHL::EResourceUsage::Default;
		_GpuClusterNodesBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GpuClusterNodesBufferDesc.initialState = GHL::EResourceState::Common;
		_GpuClusterNodesBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::CopyDestination;
		mGpuClusterNodesBuffer = resourceAllocator->Allocate(device, _GpuClusterNodesBufferDesc, descriptorAllocator, nullptr);
		mGpuClusterNodesBuffer->SetDebugName(resourceName);
		renderGraph->ImportResource(resourceName, mGpuClusterNodesBuffer);
		resourceStateTracker->StartTracking(mGpuClusterNodesBuffer);

		resourceName = mInstanceName + "_SortedInstancesBuffer";
		BufferDesc _GpuSortedInstancesBufferDesc{};
		_GpuSortedInstancesBufferDesc.stride = sizeof(uint32_t);
		_GpuSortedInstancesBufferDesc.size = _GpuSortedInstancesBufferDesc.stride * mClusterTree->sortedInstances.size();
		_GpuSortedInstancesBufferDesc.usage = GHL::EResourceUsage::Default;
		_GpuSortedInstancesBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GpuSortedInstancesBufferDesc.initialState = GHL::EResourceState::Common;
		_GpuSortedInstancesBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::CopyDestination;
		mGpuSortedInstancesBuffer = resourceAllocator->Allocate(device, _GpuSortedInstancesBufferDesc, descriptorAllocator, nullptr);
		mGpuSortedInstancesBuffer->SetDebugName(resourceName);
		renderGraph->ImportResource(resourceName, mGpuSortedInstancesBuffer);
		resourceStateTracker->StartTracking(mGpuSortedInstancesBuffer);

		resourceName = mInstanceName + "_TransformedBoundingBoxBuffer";
		BufferDesc _GpuTransformedBoundingBoxBufferDesc{};
		_GpuTransformedBoundingBoxBufferDesc.stride = sizeof(Math::BoundingBox);
		_GpuTransformedBoundingBoxBufferDesc.size = _GpuTransformedBoundingBoxBufferDesc.stride * mTransformedBoundingBoxs.size();
		_GpuTransformedBoundingBoxBufferDesc.usage = GHL::EResourceUsage::Default;
		_GpuTransformedBoundingBoxBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GpuTransformedBoundingBoxBufferDesc.initialState = GHL::EResourceState::Common;
		_GpuTransformedBoundingBoxBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::CopyDestination;
		mGpuTransformedBoundingBoxBuffer = resourceAllocator->Allocate(device, _GpuTransformedBoundingBoxBufferDesc, descriptorAllocator, nullptr);
		mGpuTransformedBoundingBoxBuffer->SetDebugName(resourceName);
		renderGraph->ImportResource(resourceName, mGpuTransformedBoundingBoxBuffer);
		resourceStateTracker->StartTracking(mGpuTransformedBoundingBoxBuffer);

		// 创建GpuCulledClusterNodesIndexBuffer、GpuCulledInstanceIndexBuffers(Lod0 Lod1 Lod2)
		resourceName = mInstanceName + "_GpuCulledVisibleClusterNodesIndexBuffer";
		BufferDesc _GpuCulledVisibleClusterNodesIndexBufferDesc{};	
		_GpuCulledVisibleClusterNodesIndexBufferDesc.stride = sizeof(uint32_t);
		_GpuCulledVisibleClusterNodesIndexBufferDesc.size = _GpuCulledVisibleClusterNodesIndexBufferDesc.stride * mClusterTree->clusterNodes.size();
		_GpuCulledVisibleClusterNodesIndexBufferDesc.usage = GHL::EResourceUsage::Default;
		_GpuCulledVisibleClusterNodesIndexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_GpuCulledVisibleClusterNodesIndexBufferDesc.initialState = GHL::EResourceState::Common;
		_GpuCulledVisibleClusterNodesIndexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
		mGpuCulledVisibleClusterNodesIndexBuffer = resourceAllocator->Allocate(device, _GpuCulledVisibleClusterNodesIndexBufferDesc, descriptorAllocator, nullptr);
		mGpuCulledVisibleClusterNodesIndexBuffer->SetDebugName(resourceName);
		renderGraph->ImportResource(resourceName, mGpuCulledVisibleClusterNodesIndexBuffer);
		resourceStateTracker->StartTracking(mGpuCulledVisibleClusterNodesIndexBuffer);

		mGpuCulledVisibleLodInstanceIndexBuffers.resize(smLodGroupSize);
		for (int32_t index = 0; index < smLodGroupSize; index++) {
			resourceName = mInstanceName + "_GpuCulledVisibleLod" + std::to_string(index) + "InstanceIndexBuffer";
			BufferDesc _GpuCulledVisibleLodInstanceIndexBufferDesc{};
			_GpuCulledVisibleLodInstanceIndexBufferDesc.stride = sizeof(uint32_t);
			_GpuCulledVisibleLodInstanceIndexBufferDesc.size = _GpuCulledVisibleLodInstanceIndexBufferDesc.stride * mClusterTree->sortedInstances.size() / 4;
			_GpuCulledVisibleLodInstanceIndexBufferDesc.usage = GHL::EResourceUsage::Default;
			_GpuCulledVisibleLodInstanceIndexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_GpuCulledVisibleLodInstanceIndexBufferDesc.initialState = GHL::EResourceState::Common;
			_GpuCulledVisibleLodInstanceIndexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
			mGpuCulledVisibleLodInstanceIndexBuffers.at(index) = resourceAllocator->Allocate(device, _GpuCulledVisibleLodInstanceIndexBufferDesc, descriptorAllocator, nullptr);
			mGpuCulledVisibleLodInstanceIndexBuffers.at(index)->SetDebugName(resourceName);
			renderGraph->ImportResource(resourceName, mGpuCulledVisibleLodInstanceIndexBuffers.at(index));
			resourceStateTracker->StartTracking(mGpuCulledVisibleLodInstanceIndexBuffers.at(index));
		}
	}

}