#include "Renderer/RaytracedTerrainShadow.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/LinearBufferAllocator.h"

namespace Renderer {

	void RaytracedTerrainShadow::Initialize(RenderEngine* renderEngine) {

	}

	void RaytracedTerrainShadow::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph->AddPass(
			"RaytracedTerrainShadowPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("GBufferPositionEmission", ShaderAccessFlag::NonPixelShader);

				NewTextureProperties _SSRaytracedTerrainShadowProperties{};
				_SSRaytracedTerrainShadowProperties.width = finalOutputDesc.width;
				_SSRaytracedTerrainShadowProperties.height = finalOutputDesc.height;
				_SSRaytracedTerrainShadowProperties.format = DXGI_FORMAT_R32_FLOAT;
				_SSRaytracedTerrainShadowProperties.clearValue = GHL::ColorClearValue{ 0.0f };
				builder.DeclareTexture("SSRaytracedTerrainShadow", _SSRaytracedTerrainShadowProperties);
				builder.WriteTexture("SSRaytracedTerrainShadow");

				shaderManger.CreateComputeShader("RaytracedTerrainShadowPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/RaytracedTerrainShadow/RaytracedTerrainShadowPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* terrainHeightMap         = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* gBufferPositionEmission  = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* ssRaytracedTerrainShadow = resourceStorage->GetResourceByName("SSRaytracedTerrainShadow")->GetTexture();
				auto& ssRaytracedTerrainShadowDesc = ssRaytracedTerrainShadow->GetResourceFormat().GetTextureDesc();

				mRaytracedTerrainShadowPassData.terrainHeightMapIndex = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
				mRaytracedTerrainShadowPassData.gBufferPositionEmissionMapIndex  = gBufferPositionEmission->GetSRDescriptor()->GetHeapIndex();
				mRaytracedTerrainShadowPassData.ssRaytracedTerrainShadowMapIndex = ssRaytracedTerrainShadow->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(RaytracedTerrainShadowPassData));
				memcpy(passDataAlloc.cpuAddress, &mRaytracedTerrainShadowPassData, sizeof(RaytracedTerrainShadowPassData));

				uint32_t threadGroupCountX = (ssRaytracedTerrainShadowDesc.width + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (ssRaytracedTerrainShadowDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("RaytracedTerrainShadowPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetComputeRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			}
			);
	}

}