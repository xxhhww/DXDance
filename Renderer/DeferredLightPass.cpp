#include "Renderer/DeferredLightPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"

#include "Math/Halton.h"

namespace Renderer {

	void DeferredLightPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"DeferredLightPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("RngSeedMap", ShaderAccessFlag::NonPixelShader);
				// builder.ReadTexture("BlueNoise3DMap", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("SkyLuminance", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferAlbedoMetalness", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferPositionEmission", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferNormalRoughness", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferViewDepth", ShaderAccessFlag::NonPixelShader);
				builder.WriteTexture("FinalOutput");

				NewTextureProperties _DeferredLightingRayPDFsProperties;
				_DeferredLightingRayPDFsProperties.width = finalOutputDesc.width;
				_DeferredLightingRayPDFsProperties.height = finalOutputDesc.height;
				_DeferredLightingRayPDFsProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				builder.DeclareTexture("DeferredLightingRayPDFs", _DeferredLightingRayPDFsProperties);
				builder.WriteTexture("DeferredLightingRayPDFs");

				NewTextureProperties _DeferredLightingRayLightIntersectionPointsProperties;
				_DeferredLightingRayLightIntersectionPointsProperties.width = finalOutputDesc.width;
				_DeferredLightingRayLightIntersectionPointsProperties.height = finalOutputDesc.height;
				_DeferredLightingRayLightIntersectionPointsProperties.format = DXGI_FORMAT_R32G32B32A32_UINT;
				builder.DeclareTexture("DeferredLightingRayLightIntersectionPoints", _DeferredLightingRayLightIntersectionPointsProperties);
				builder.WriteTexture("DeferredLightingRayLightIntersectionPoints");

				shaderManger.CreateComputeShader("DeferredLightPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/DeferredLightPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* rngSeedMap              = resourceStorage->GetResourceByName("RngSeedMap")->GetTexture();
				auto* blueNoise3DMap          = resourceStorage->GetResourceByName("BlueNoise3DMap")->GetTexture();
				auto* skyLuminance            = resourceStorage->GetResourceByName("SkyLuminance")->GetTexture();
				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* finalOutput             = resourceStorage->GetResourceByName("FinalOutput")->GetTexture();
				auto& finalOutputDesc = finalOutput->GetResourceFormat().GetTextureDesc();

				deferredLightPassData.halton                           = Math::Halton::Sequence(0, 3).data();
				deferredLightPassData.rngSeedMapIndex                  = rngSeedMap->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.blueNoise3DMapIndex              = blueNoise3DMap->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.skyLuminanceMapIndex             = skyLuminance->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferAlbedoMetalnessMapIndex   = gBufferAlbedoMetalness->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferPositionEmissionMapIndex  = gBufferPositionEmission->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferNormalRoughnessMapIndex   = gBufferNormalRoughness->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.gBufferViewDepthMapIndex         = gBufferViewDepth->GetSRDescriptor()->GetHeapIndex();
				deferredLightPassData.finalOutputMapIndex              = finalOutput->GetUADescriptor()->GetHeapIndex();
				deferredLightPassData.finalOutputMapSizeX              = finalOutputDesc.width;
				deferredLightPassData.finalOutputMapSizeY              = finalOutputDesc.height;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(DeferredLightPassData));
				memcpy(passDataAlloc.cpuAddress, &deferredLightPassData, sizeof(DeferredLightPassData));

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