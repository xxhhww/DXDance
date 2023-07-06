#include "Renderer/GenerateCloudNoiseTask.h"
#include "Renderer/FixedTextureHelper.h"

namespace Renderer {

	void GenerateCloudNoiseTask::Initialize(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();

		{
			mDevice = device; 
		}

		{
			Renderer::TextureDesc _ShapeNoiseMapDesc{};
			_ShapeNoiseMapDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_ShapeNoiseMapDesc.dimension = GHL::ETextureDimension::Texture3D;
			_ShapeNoiseMapDesc.width = 128u;
			_ShapeNoiseMapDesc.height = 128u;
			_ShapeNoiseMapDesc.depth = 128u;
			_ShapeNoiseMapDesc.mipLevals = 1u;
			_ShapeNoiseMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
			_ShapeNoiseMapDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::CopySource;
			mShapeNoiseMap = resourceAllocator->Allocate(device, _ShapeNoiseMapDesc, descriptorAllocator, nullptr);
		
			Renderer::BufferDesc _ShapeNoiseMapReadbackDesc{};
			_ShapeNoiseMapReadbackDesc.size = Math::AlignUp(
				mShapeNoiseMap->GetResourceFormat().GetSizeInBytes(),
				D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
			);
			_ShapeNoiseMapReadbackDesc.stride = 0u;
			_ShapeNoiseMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_ShapeNoiseMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_ShapeNoiseMapReadbackDesc.expectedState = _ShapeNoiseMapReadbackDesc.initialState;
			mShapeNoiseMapReadback = resourceAllocator->Allocate(device, _ShapeNoiseMapReadbackDesc, descriptorAllocator, nullptr);
		}

		{
			Renderer::TextureDesc _DetailNoiseMapDesc{};
			_DetailNoiseMapDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_DetailNoiseMapDesc.dimension = GHL::ETextureDimension::Texture3D;
			_DetailNoiseMapDesc.width = 32u;
			_DetailNoiseMapDesc.height = 32u;
			_DetailNoiseMapDesc.depth = 32u;
			_DetailNoiseMapDesc.mipLevals = 1u;
			_DetailNoiseMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
			_DetailNoiseMapDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::CopySource;
			mDetailNoiseMap = resourceAllocator->Allocate(device, _DetailNoiseMapDesc, descriptorAllocator, nullptr);
		
			Renderer::BufferDesc _DetailNoiseMapReadbackDesc{};
			_DetailNoiseMapReadbackDesc.size = Math::AlignUp(
				mDetailNoiseMap->GetResourceFormat().GetSizeInBytes(),
				D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
			);
			_DetailNoiseMapReadbackDesc.stride = 0u;
			_DetailNoiseMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_DetailNoiseMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_DetailNoiseMapReadbackDesc.expectedState = _DetailNoiseMapReadbackDesc.initialState;
			mDetailNoiseMapReadback = resourceAllocator->Allocate(device, _DetailNoiseMapReadbackDesc, descriptorAllocator, nullptr);
		}

		// 创建Shader
		{
			shaderManger->CreateComputeShader(
				"ShapeNoiseGenerator",
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/VolumetricClouds/ShapeNoiseGenerator.hlsl";
				}
			);

			shaderManger->CreateComputeShader(
				"DetailNoiseGenerator",
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/VolumetricClouds/DetailNoiseGenerator.hlsl";
				}
			);
		}

		// 追踪资源状态
		{
			resourceStateTracker->StartTracking(mShapeNoiseMap.Get());
			resourceStateTracker->StartTracking(mDetailNoiseMap.Get());
			resourceStateTracker->StartTracking(mShapeNoiseMapReadback.Get());
			resourceStateTracker->StartTracking(mDetailNoiseMapReadback.Get());
		}
	}

	void GenerateCloudNoiseTask::Generate(CommandBuffer& commandBuffer, RenderContext& renderContext) {
		auto* device = renderContext.device;
		auto* dynamicAllocator = renderContext.dynamicAllocator;
		commandBuffer.SetComputeRootSignature();

		// First Pass: Generate ShapeNoiseMap
		{
			auto& shapeNoiseMapDesc = mShapeNoiseMap->GetResourceFormat().GetTextureDesc();
			shapeNoiseGeneratorPassData.shapeNoiseMapIndex  = mShapeNoiseMap->GetUADescriptor()->GetHeapIndex();
			shapeNoiseGeneratorPassData.shapeNoiseMapWidth  = shapeNoiseMapDesc.width;
			shapeNoiseGeneratorPassData.shapeNoiseMapHeight = shapeNoiseMapDesc.height;
			shapeNoiseGeneratorPassData.shapeNoiseMapDepth  = shapeNoiseMapDesc.depth;

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(ShapeNoiseGeneratorPassData));
			memcpy(passDataAlloc.cpuAddress, &shapeNoiseGeneratorPassData, sizeof(ShapeNoiseGeneratorPassData));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mShapeNoiseMap.Get(), GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t threadGroupCountX = (shapeNoiseMapDesc.width  + smThreadSizeInGroup1 - 1u) / smThreadSizeInGroup1;
			uint32_t threadGroupCountY = (shapeNoiseMapDesc.height + smThreadSizeInGroup1 - 1u) / smThreadSizeInGroup1;
			uint32_t threadGroupCountZ = (shapeNoiseMapDesc.depth  + smThreadSizeInGroup1 - 1u) / smThreadSizeInGroup1;

			commandBuffer.SetComputePipelineState("ShapeNoiseGenerator");
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
		}

		// Second Pass: Copy ShapeNoiseMap From VideoMemory To SharedMemory
		{
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mShapeNoiseMap.Get(), GHL::EResourceState::CopySource);
			barrierBatch += commandBuffer.TransitionImmediately(mShapeNoiseMapReadback.Get(), GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t subresourceCount = mShapeNoiseMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizeInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = mShapeNoiseMap->GetResourceFormat().D3DResourceDesc();
			device->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
				placedLayouts.data(), numRows.data(), rowSizeInBytes.data(), &requiredSize);

			for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
				auto& placedLayout = placedLayouts.at(subresourceIndex);

				// 将数据从显存复制到共享内存
				auto rbPlacedLayout = placedLayout;
				rbPlacedLayout.Footprint.RowPitch = (rbPlacedLayout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

				D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mShapeNoiseMap->D3DResource(), subresourceIndex);
				D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mShapeNoiseMapReadback->D3DResource(), rbPlacedLayout);

				commandBuffer.D3DCommandList()->CopyTextureRegion(
					&dstLocation,
					0u, 0u, 0u,
					&srcLocation,
					nullptr
				);
			}
		}


		// Third Pass: Generate DetailNoiseMap
		{
			auto& detailNoiseMapDesc = mDetailNoiseMap->GetResourceFormat().GetTextureDesc();
			detailNoiseGeneratorPassData.detailNoiseMapIndex  = mDetailNoiseMap->GetUADescriptor()->GetHeapIndex();
			detailNoiseGeneratorPassData.detailNoiseMapWidth  = detailNoiseMapDesc.width;
			detailNoiseGeneratorPassData.detailNoiseMapHeight = detailNoiseMapDesc.height;
			detailNoiseGeneratorPassData.detailNoiseMapDepth  = detailNoiseMapDesc.depth;

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(DetailNoiseGeneratorPassData));
			memcpy(passDataAlloc.cpuAddress, &detailNoiseGeneratorPassData, sizeof(DetailNoiseGeneratorPassData));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mDetailNoiseMap.Get(), GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t threadGroupCountX = (detailNoiseMapDesc.width  + smThreadSizeInGroup2 - 1u) / smThreadSizeInGroup2;
			uint32_t threadGroupCountY = (detailNoiseMapDesc.height + smThreadSizeInGroup2 - 1u) / smThreadSizeInGroup2;
			uint32_t threadGroupCountZ = (detailNoiseMapDesc.depth  + smThreadSizeInGroup2 - 1u) / smThreadSizeInGroup2;
		
			commandBuffer.SetComputePipelineState("DetailNoiseGenerator");
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
		}

		// Fourth Pass: Copy DetailNoiseMap From VideoMemory To SharedMemory
		{
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mDetailNoiseMap.Get(), GHL::EResourceState::CopySource);
			barrierBatch += commandBuffer.TransitionImmediately(mDetailNoiseMapReadback.Get(), GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t subresourceCount = mDetailNoiseMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizeInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = mDetailNoiseMap->GetResourceFormat().D3DResourceDesc();
			device->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
				placedLayouts.data(), numRows.data(), rowSizeInBytes.data(), &requiredSize);

			for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
				auto& placedLayout = placedLayouts.at(subresourceIndex);

				// 将数据从显存复制到共享内存
				auto rbPlacedLayout = placedLayout;
				rbPlacedLayout.Footprint.RowPitch = (rbPlacedLayout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

				D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mDetailNoiseMap->D3DResource(), subresourceIndex);
				D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mDetailNoiseMapReadback->D3DResource(), rbPlacedLayout);

				commandBuffer.D3DCommandList()->CopyTextureRegion(
					&dstLocation,
					0u, 0u, 0u,
					&srcLocation,
					nullptr
				);
			}
		}
	}

	void GenerateCloudNoiseTask::OnCompleted() {
		// 离线任务完成，将结果保存到磁盘中

		// Save ShapeNoiseMap
		{
			FixedTextureHelper::SaveToFile(mDevice, mShapeNoiseMap, mShapeNoiseMapReadback,
				"ShapeNoiseMap.dds");
		}

		// Save DetailNoiseMap
		{
			FixedTextureHelper::SaveToFile(mDevice, mDetailNoiseMap, mDetailNoiseMapReadback,
				"DetailNoiseMap.dds");
		}
	}

}