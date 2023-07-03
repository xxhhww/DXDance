#include "Renderer/VolumetricCloudsPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/FixedTextureHelper.h"

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

				auto* blueNoise2DMap          = resourceStorage->GetResourceByName("BlueNoise2DMap")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* deferredLightshadingOut = resourceStorage->GetResourceByName("DeferredLightShadingOut")->GetTexture();
				
				auto& blueNoise2DMapDesc          = blueNoise2DMap->GetResourceFormat().GetTextureDesc();
				auto& deferredLightshadingOutDesc = deferredLightshadingOut->GetResourceFormat().GetTextureDesc();

				volumetricCloudsMainPassData.weatherMapIndex			= weatherMap->GetSRDescriptor()->GetHeapIndex();
				volumetricCloudsMainPassData.blueNoise2DMapIndex        = blueNoise2DMap->GetSRDescriptor()->GetHeapIndex();
				volumetricCloudsMainPassData.blueNoise2DMapWidth        = blueNoise2DMapDesc.width;
				volumetricCloudsMainPassData.blueNoise2DMapHeight       = blueNoise2DMapDesc.height;
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
		auto* copyQueue			    = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence				= renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator	= renderEngine->mDescriptorAllocator.get();
		auto* resourceAllocator     = renderEngine->mResourceAllocator.get();
		auto* resourceStateTracker	= renderEngine->mResourceStateTracker.get();
		auto* renderGraph			= renderEngine->mRenderGraph.get();

		// Load Weather From File
		{
			weatherMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyQueue, copyFence, 
				"E:/MyProject/DXDance/Resources/Textures/VolumetricClouds/Weather.dds");
			resourceStateTracker->StartTracking(weatherMap.Get());
		}
	}

}