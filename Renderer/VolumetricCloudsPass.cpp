#include "Renderer/VolumetricCloudsPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

#include "Renderer/FormatConverter.h"

#include "DirectXTex/DirectXTex.h"

#include "GHL/Box.h"

namespace Renderer {

	void VolumetricCloudsPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"VolumetricCloudsPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("GBufferViewDepth", ShaderAccessFlag::NonPixelShader);
				builder.WriteTexture("DeferredLightShadingOut");

				shaderManger.CreateComputeShader("VolumetricCloudsPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/VolumetricClouds/VolumetricCloudsPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* rawWeatherMap           = weatherMap.get();
				auto* rawCloudMap             = cloudMap.get();
				auto* rawWorleyMap            = worleyMap.get();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* deferredLightshadingOut = resourceStorage->GetResourceByName("DeferredLightShadingOut")->GetTexture();
				auto& deferredLightshadingOutDesc = deferredLightshadingOut->GetResourceFormat().GetTextureDesc();

				// volumetricCloudsMainPassData.weatherMapIndex			= rawWeatherMap->GetSRDescriptor()->GetHeapIndex();
				// volumetricCloudsMainPassData.cloudMapIndex			= rawCloudMap->GetSRDescriptor()->GetHeapIndex();
				// volumetricCloudsMainPassData.worleyMapIndex			= rawWorleyMap->GetSRDescriptor()->GetHeapIndex();
				volumetricCloudsMainPassData.gBufferViewDepthMapIndex	= gBufferViewDepth->GetSRDescriptor()->GetHeapIndex();
				volumetricCloudsMainPassData.previousPassOutputMapIndex = deferredLightshadingOut->GetUADescriptor()->GetHeapIndex();
				volumetricCloudsMainPassData.previousPassOutputWidth	= deferredLightshadingOutDesc.width;
				volumetricCloudsMainPassData.previousPassOutputHeight	= deferredLightshadingOutDesc.height;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VolumetricCloudsMainPassData));
				memcpy(passDataAlloc.cpuAddress, &volumetricCloudsMainPassData, sizeof(VolumetricCloudsMainPassData));

				uint32_t threadGroupCountX = (deferredLightshadingOutDesc.width  + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (deferredLightshadingOutDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("VolumetricCloudsPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetComputeRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			}
			);

		/*
		renderGraph.AddPass(
			"VolumetricCloudsBlurPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
			}
			);
		*/
	}

	void VolumetricCloudsPass::InitializePass(RenderEngine* renderEngine) {
		auto* device				= renderEngine->mDevice.get();
		auto* copyDsQueue			= renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence				= renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator	= renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker	= renderEngine->mResourceStateTracker.get();
		auto* renderGraph			= renderEngine->mRenderGraph.get();

		// Load cloud From Memory(目前读取的DDS文件，后续将DDS转换为XET)
		/*
		{
			DirectX::ScratchImage baseImage;
			HRASSERT(DirectX::LoadFromDDSFile(
				L"E:/MyProject/DXDance/Resources/Textures/VolumetricClouds/cloud.dds",
				DirectX::DDS_FLAGS_NONE,
				nullptr,
				baseImage
			));


			Renderer::TextureDesc _CloudMapDesc = FormatConverter::GetTextureDesc(baseImage.GetMetadata());
			_CloudMapDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess;
			cloudMap = std::make_unique<Texture>(
				device,
				ResourceFormat{ device, _CloudMapDesc },
				descriptorAllocator,
				nullptr
				);

			// 获取纹理GPU存储信息
			uint32_t subresourceCount = cloudMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizesInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = cloudMap->GetResourceFormat().D3DResourceDesc();
			device->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, 1u, 0u,
				placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

			// 上传数据至显存
			uint8_t* temp = new uint8_t[requiredSize];
			for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
				for (uint32_t sliceIndex = 0u; sliceIndex < d3dResDesc.DepthOrArraySize; sliceIndex++) {
					auto* image = baseImage.GetImage(0u, 0u, sliceIndex);

					for (uint32_t rowIndex = 0u; rowIndex < numRows[subresourceIndex]; rowIndex++) {
						uint32_t realByteOffset = rowIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch + sliceIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch * numRows[subresourceIndex];
						uint32_t fakeByteOffset = rowIndex * image->rowPitch;
						memcpy(temp + realByteOffset, image->pixels + fakeByteOffset, image->rowPitch);
					}
				}

				DSTORAGE_REQUEST request = {};
				request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
				request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
				request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
				request.Source.Memory.Source = temp;
				request.Source.Memory.Size = requiredSize;
				request.Destination.Texture.Resource = cloudMap->D3DResource();
				request.Destination.Texture.Region = GHL::Box{
					0u, placedLayouts.at(subresourceIndex).Footprint.Width,
					0u, placedLayouts.at(subresourceIndex).Footprint.Height,
					0u, placedLayouts.at(subresourceIndex).Footprint.Depth
				}.D3DBox();
				request.Destination.Texture.SubresourceIndex = subresourceIndex;
				request.UncompressedSize = requiredSize;
				copyDsQueue->EnqueueRequest(&request);
			}
			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();
			baseImage.Release();

			resourceStateTracker->StartTracking(cloudMap.get());
		}
		*/
	}

}