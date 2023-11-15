#include "Renderer/HierarchyInstancedStaticMesh.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"
#include "Tools/Assert.h"

namespace Renderer {

	HoudiniInstanceData::HoudiniInstanceData(
		const Math::Vector3& _position,
		const Math::Vector4& _quaternion,
		const Math::Vector3& _scaling) {

		/*
		position = Math::Vector3{ _position.x, _position.y, _position.z };
		quaternion = Math::Vector4{ _quaternion.x, _quaternion.y, _quaternion.z, _quaternion.w };
		scaling = Math::Vector3{ _scaling.x, _scaling.y, _scaling.z };
		*/

		position = Math::Vector3{ _position.z, _position.y, _position.x };
		quaternion = Math::Vector4{ _quaternion.z, _quaternion.y, _quaternion.x, _quaternion.w };
		scaling = Math::Vector3{ _scaling.z, _scaling.y, _scaling.x };
	}

	HierarchyInstancedStaticMesh::HierarchyInstancedStaticMesh(
		RenderEngine* renderEngine,
		const std::string& instanceName,
		const std::string& instancePath,
		int32_t instanceCountPerCluster,
		int32_t instanceVisibleDistance,
		int32_t instanceLodGroupSize,
		const std::vector<float>& instanceLodDistancesScale)
		: mRenderEngine(renderEngine)
		, mInstanceName(instanceName)
		, mInstancePath(instancePath)
		, mInstanceCountPerCluster(instanceCountPerCluster)
		, mInstanceVisibleDistance(instanceVisibleDistance)
		, mLodGroupSize(instanceLodGroupSize) 
		, mLodDistancesScale(instanceLodDistancesScale) {

		ASSERT_FORMAT(instanceLodGroupSize == instanceLodDistancesScale.size(), "Incorrect LodGroupSize");

		Initialize();
	}

	void HierarchyInstancedStaticMesh::BuildTree() {
		std::vector<Math::Matrix4> cpuTransforms;
		cpuTransforms.resize(mTransforms.size());
		for (int32_t i = 0; i < mTransforms.size(); i++) {
			cpuTransforms[i] = mTransforms[i].Transpose();
		}

		Renderer::ClusterBuilder treeBuilder(cpuTransforms, mInstanceBoundingBox, mInstanceCountPerCluster);
		treeBuilder.BuildTree(true);	// 只构建叶子节点

		mClusterTree = std::move(treeBuilder.result);
	}

	void HierarchyInstancedStaticMesh::Initialize() {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStorage = mRenderEngine->mPipelineResourceStorage;
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();
		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		mLodDistances.resize(mLodGroupSize);
		for (int32_t index = 0; index < mLodGroupSize; index++) {
			mLodDistances.at(index) = mInstanceVisibleDistance * mLodDistancesScale.at(index);
		}

		// LodGroups
		for (int32_t index = 0; index < mLodGroupSize; index++) {
			std::string lodPath = mInstancePath + "/Lod" + std::to_string(index) + ".fbx";
			std::unique_ptr<Renderer::Model> lodModel = std::make_unique<Renderer::Model>(device, descriptorAllocator, nullptr, lodPath, Math::Vector3{ 0.015f, 0.015f, 0.015f });
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
		std::ifstream instanceDataStream(instanceDataPath, std::ios::in | std::ios::binary);
		Tool::InputMemoryStream inputStream(instanceDataStream);

		int32_t instanceCount = inputStream.Size() / sizeof(HoudiniInstanceData);
		mTransforms.resize(instanceCount);
		mTransformedBoundingBoxs.resize(instanceCount);
		for (int32_t i = 0; i < instanceCount; i++) {
			Math::Vector3 tempPosition;
			Math::Vector4 tempQuaternion;
			Math::Vector3 tempScaling;
			inputStream.Read(tempPosition);
			inputStream.Read(tempQuaternion);
			inputStream.Read(tempScaling);

			HoudiniInstanceData houdiniInstanceData(tempPosition, tempQuaternion, tempScaling);
			mTransforms[i] = Math::Matrix4(houdiniInstanceData.position, houdiniInstanceData.quaternion, houdiniInstanceData.scaling).Transpose();	// 注意转置
			mTransformedBoundingBoxs[i] = mInstanceBoundingBox.transformBy(mTransforms[i].Transpose());
		}

		// 构建集群树
		BuildTree();

		// 创建并上传transforms、transformedBoundingBoxs、clusterTree.clusterNodes 和 clusterTree.sortedInstances至GPU
		std::string resourceName = "";
		{
			resourceName = mInstanceName + "_TransformsBuffer";
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

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			dsRequest.Source.Memory.Source = static_cast<void*>(mTransforms.data());
			dsRequest.Source.Memory.Size = _GpuTransformsBufferDesc.size;
			dsRequest.Destination.Buffer.Resource = mGpuTransformsBuffer->D3DResource();
			dsRequest.Destination.Buffer.Offset = 0u;
			dsRequest.Destination.Buffer.Size = _GpuTransformsBufferDesc.size;
			dsRequest.UncompressedSize = _GpuTransformsBufferDesc.size;
			copyDsQueue->EnqueueRequest(&dsRequest);
		}

		{
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

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			dsRequest.Source.Memory.Source = static_cast<void*>(mClusterTree->clusterNodes.data());
			dsRequest.Source.Memory.Size = _GpuClusterNodesBufferDesc.size;
			dsRequest.Destination.Buffer.Resource = mGpuClusterNodesBuffer->D3DResource();
			dsRequest.Destination.Buffer.Offset = 0u;
			dsRequest.Destination.Buffer.Size = _GpuClusterNodesBufferDesc.size;
			dsRequest.UncompressedSize = _GpuClusterNodesBufferDesc.size;
			copyDsQueue->EnqueueRequest(&dsRequest);
		}

		{
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

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			dsRequest.Source.Memory.Source = static_cast<void*>(mClusterTree->sortedInstances.data());
			dsRequest.Source.Memory.Size = _GpuSortedInstancesBufferDesc.size;
			dsRequest.Destination.Buffer.Resource = mGpuSortedInstancesBuffer->D3DResource();
			dsRequest.Destination.Buffer.Offset = 0u;
			dsRequest.Destination.Buffer.Size = _GpuSortedInstancesBufferDesc.size;
			dsRequest.UncompressedSize = _GpuSortedInstancesBufferDesc.size;
			copyDsQueue->EnqueueRequest(&dsRequest);
		}

		{
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

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
			dsRequest.Source.Memory.Source = static_cast<void*>(mTransformedBoundingBoxs.data());
			dsRequest.Source.Memory.Size = _GpuTransformedBoundingBoxBufferDesc.size;
			dsRequest.Destination.Buffer.Resource = mGpuTransformedBoundingBoxBuffer->D3DResource();
			dsRequest.Destination.Buffer.Offset = 0u;
			dsRequest.Destination.Buffer.Size = _GpuTransformedBoundingBoxBufferDesc.size;
			dsRequest.UncompressedSize = _GpuTransformedBoundingBoxBufferDesc.size;
			copyDsQueue->EnqueueRequest(&dsRequest);
		}

		{
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
			resourceStateTracker->StartTracking(mGpuCulledVisibleClusterNodesIndexBuffer->GetCounterBuffer());
		}

		{
			mGpuCulledVisibleInstanceIndexBuffers.resize(mLodGroupSize);
			for (int32_t index = 0; index < mLodGroupSize; index++) {
				resourceName = mInstanceName + "_GpuCulledVisibleLod" + std::to_string(index) + "InstanceIndexBuffer";
				BufferDesc _GpuCulledVisibleLodInstanceIndexBufferDesc{};
				_GpuCulledVisibleLodInstanceIndexBufferDesc.stride = sizeof(uint32_t);
				_GpuCulledVisibleLodInstanceIndexBufferDesc.size = _GpuCulledVisibleLodInstanceIndexBufferDesc.stride * (mClusterTree->sortedInstances.size() / 3u);
				_GpuCulledVisibleLodInstanceIndexBufferDesc.usage = GHL::EResourceUsage::Default;
				_GpuCulledVisibleLodInstanceIndexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GpuCulledVisibleLodInstanceIndexBufferDesc.initialState = GHL::EResourceState::Common;
				_GpuCulledVisibleLodInstanceIndexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
				mGpuCulledVisibleInstanceIndexBuffers.at(index) = resourceAllocator->Allocate(device, _GpuCulledVisibleLodInstanceIndexBufferDesc, descriptorAllocator, nullptr);
				mGpuCulledVisibleInstanceIndexBuffers.at(index)->SetDebugName(resourceName);
				renderGraph->ImportResource(resourceName, mGpuCulledVisibleInstanceIndexBuffers.at(index));
				resourceStateTracker->StartTracking(mGpuCulledVisibleInstanceIndexBuffers.at(index));
				resourceStateTracker->StartTracking(mGpuCulledVisibleInstanceIndexBuffers.at(index)->GetCounterBuffer());
			}
		}

		copyFence->IncrementExpectedValue();
		copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
		copyDsQueue->Submit();
		copyFence->Wait();
	}

}