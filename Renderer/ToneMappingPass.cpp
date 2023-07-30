#include "Renderer/ToneMappingPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"

#include "GHL/Display.h"

namespace Renderer {

	void ToneMappingPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"ToneMappingPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("TAACurrentFrameOutput", ShaderAccessFlag::NonPixelShader);
				builder.WriteTexture("FinalOutput");
				
				shaderManger.CreateComputeShader("ToneMappingPass",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ToneMapping/ToneMappingPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;
				auto* display = renderContext.display;

				auto* taaCurrentFrameOutput   = resourceStorage->GetResourceByName("TAACurrentFrameOutput")->GetTexture();
				auto* finalOutput             = resourceStorage->GetResourceByName("FinalOutput")->GetTexture();
				auto& deferredLightshadingOutDesc = taaCurrentFrameOutput->GetResourceFormat().GetTextureDesc();

				toneMappingPassData.inputMapIndex		= taaCurrentFrameOutput->GetSRDescriptor()->GetHeapIndex();
				toneMappingPassData.outputMapIndex		= finalOutput->GetUADescriptor()->GetHeapIndex();
				toneMappingPassData.isHDREnabled		= false;
				toneMappingPassData.displayMaxLuminance = display->maxLuminance;
				toneMappingPassData.tonemappingParams;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(ToneMappingPass));
				memcpy(passDataAlloc.cpuAddress, &toneMappingPassData, sizeof(ToneMappingPass));

				uint32_t threadGroupCountX = (deferredLightshadingOutDesc.width  + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (deferredLightshadingOutDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("ToneMappingPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});
	}

	void ToneMappingPass::AddForwardPlusPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"ToneMappingPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("TAACurrentFrameOutput", ShaderAccessFlag::NonPixelShader);
				builder.WriteTexture("FinalOutput");

				shaderManger.CreateComputeShader("ToneMappingPass",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ToneMapping/ToneMappingPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;
				auto* display = renderContext.display;

				auto* taaCurrentFrameOutput = resourceStorage->GetResourceByName("TAACurrentFrameOutput")->GetTexture();
				auto* finalOutput = resourceStorage->GetResourceByName("FinalOutput")->GetTexture();
				auto& deferredLightshadingOutDesc = taaCurrentFrameOutput->GetResourceFormat().GetTextureDesc();

				toneMappingPassData.inputMapIndex = taaCurrentFrameOutput->GetSRDescriptor()->GetHeapIndex();
				toneMappingPassData.outputMapIndex = finalOutput->GetUADescriptor()->GetHeapIndex();
				toneMappingPassData.isHDREnabled = false;
				toneMappingPassData.displayMaxLuminance = display->maxLuminance;
				toneMappingPassData.tonemappingParams;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(ToneMappingPass));
				memcpy(passDataAlloc.cpuAddress, &toneMappingPassData, sizeof(ToneMappingPass));

				uint32_t threadGroupCountX = (deferredLightshadingOutDesc.width + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (deferredLightshadingOutDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("ToneMappingPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});
	}

}