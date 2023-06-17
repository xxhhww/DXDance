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

				builder.ReadTexture("GBufferAlbedoMetalness", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferPositionEmission", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferNormalRoughness", ShaderAccessFlag::NonPixelShader);
				builder.ReadDepthStencil("GBufferViewDepth");
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

				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* finalOutput             = resourceStorage->GetResourceByName("FinalOutput")->GetTexture();

				deferredLightPassData.gBufferAlbedoMetalnessMapIndex   = gBufferAlbedoMetalness->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferPositionEmissionMapIndex  = gBufferPositionEmission->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferNormalRoughnessMapIndex   = gBufferNormalRoughness->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferViewDepthMapIndex         = gBufferViewDepth->GetDSDescriptor()->GetHeapIndex();
				deferredLightPassData.finalOutputMapIndex              = finalOutput->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(DeferredLightPassData));
				memcpy(passDataAlloc.cpuAddress, &deferredLightPassData, sizeof(DeferredLightPassData));

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