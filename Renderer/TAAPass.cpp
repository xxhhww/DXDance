#include "Renderer/TAAPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"

namespace Renderer {

	void TAAPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"TAAPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("DeferredLightShadingOut", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferMotionVector", ShaderAccessFlag::NonPixelShader);
				NewTextureProperties _TAAOutputProperties{};
				_TAAOutputProperties.width  = finalOutputDesc.width;
				_TAAOutputProperties.height = finalOutputDesc.height;
				_TAAOutputProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_TAAOutputProperties.aliased = false;
				builder.DeclareTexture("TAAOutput_1", _TAAOutputProperties);
				builder.WriteTexture("TAAOutput_1");
				builder.DeclareTexture("TAAOutput_2", _TAAOutputProperties);
				builder.WriteTexture("TAAOutput_2");

				shaderManger.CreateComputeShader("TAAPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/TAAPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* gBufferMotionVector     = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* deferredLightShadingOut = resourceStorage->GetResourceByName("DeferredLightShadingOut")->GetTexture();

				uint32_t threadGroupCountX = (finalOutputDesc.width  + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (finalOutputDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				taaPassData.dispatchGroupCountX         = threadGroupCountX;
				taaPassData.dispatchGroupCountY         = threadGroupCountY;
				taaPassData.previousTAAOutputMapIndex;
				taaPassData.previousPassOutputMapIndex	= deferredLightShadingOut->GetSRDescriptor()->GetHeapIndex();
				taaPassData.motionMapIndex              = gBufferMotionVector->GetSRDescriptor()->GetHeapIndex();
				taaPassData.outputMapIndex;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TAAPass));
				memcpy(passDataAlloc.cpuAddress, &taaPassData, sizeof(TAAPass));

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TAAPass");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});
	}

}