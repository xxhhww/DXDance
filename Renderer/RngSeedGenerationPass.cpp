#include "Renderer/RngSeedGenerationPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/RingFrameTracker.h"

namespace Renderer {

	void RngSeedGenerationPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		auto& blueNoise3DMapDesc = 
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("BlueNoise3DMap")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"RngSeedGenerationPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewTextureProperties _RngSeedMapProperties{};
				_RngSeedMapProperties.width = finalOutputDesc.width;
				_RngSeedMapProperties.height = finalOutputDesc.height;
				_RngSeedMapProperties.format = DXGI_FORMAT_R8G8B8A8_UINT;
				builder.DeclareTexture("RngSeedMap", _RngSeedMapProperties);
				builder.WriteTexture("RngSeedMap");

				shaderManger.CreateComputeShader("RngSeedGenerationPass",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/RngSeedGenerationPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* frameTracker = renderContext.frameTracker;

				auto* rngSeedMap   = resourceStorage->GetResourceByName("RngSeedMap")->GetTexture();

				rngSeedGenerationPassData.rngSeedMapIndex     = rngSeedMap->GetUADescriptor()->GetHeapIndex();
				rngSeedGenerationPassData.blueNoise3DMapSize  = blueNoise3DMapDesc.width;
				rngSeedGenerationPassData.blueNoise3DMapDepth = blueNoise3DMapDesc.depth;
				rngSeedGenerationPassData.currFrameNumber     = frameTracker->GetCurrFrameNumber();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(RngSeedGenerationPassData));
				memcpy(passDataAlloc.cpuAddress, &rngSeedGenerationPassData, sizeof(RngSeedGenerationPassData));

				auto& rngSeedMapDesc = rngSeedMap->GetResourceFormat().GetTextureDesc();
				uint32_t threadGroupCountX = (rngSeedMapDesc.width + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (rngSeedMapDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("RngSeedGenerationPass");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});

	}

}