#include "Renderer/DeferredLightPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"

namespace Renderer {

	void DeferredLightPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"DeferredLightPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("GBufferAlbedo", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferPosition", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferNormal", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferMRE", ShaderAccessFlag::NonPixelShader);
				builder.WriteTexture("FinalOutput");

				shaderManger.CreateComputeShader("DeferredLightPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/DeferredLightPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* gbufferAlbedo = resourceStorage->GetResourceByName("GBufferAlbedo")->GetTexture();
				auto* gbufferPosition = resourceStorage->GetResourceByName("GBufferPosition")->GetTexture();
				auto* gbufferNormal = resourceStorage->GetResourceByName("GBufferNormal")->GetTexture();
				auto* gbufferMRE = resourceStorage->GetResourceByName("GBufferMRE")->GetTexture();
				auto* finalOutput = resourceStorage->GetResourceByName("FinalOutput")->GetTexture();

				_DeferredLightPassData._GBufferAlbedoIndex   = gbufferAlbedo->GetSRDescriptor()->GetHeapIndex();
				_DeferredLightPassData._GBufferPositionIndex = gbufferPosition->GetSRDescriptor()->GetHeapIndex();
				_DeferredLightPassData._GBufferNormalIndex   = gbufferNormal->GetSRDescriptor()->GetHeapIndex();
				_DeferredLightPassData._GBufferMREIndex      = gbufferMRE->GetSRDescriptor()->GetHeapIndex();
				_DeferredLightPassData._FinalOutputIndex     = finalOutput->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(DeferredLightPassData));
				memcpy(passDataAlloc.cpuAddress, &_DeferredLightPassData, sizeof(DeferredLightPassData));

				auto& finalOutputDesc = finalOutput->GetResourceFormat().GetTextureDesc();
				uint32_t threadGroupCountX = (finalOutputDesc.width + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (finalOutputDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("DeferredLightPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetComputeRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});

	}

}