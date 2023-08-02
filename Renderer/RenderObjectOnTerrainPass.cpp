#include "Renderer/RenderObjectOnTerrainPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	struct IndirectDraw {
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_DRAW_ARGUMENTS         drawArguments;
	};

	void RenderObjectOnTerrainPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"RenderGrassOnTerrain",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("GrassPlacementBuffer", ShaderAccessFlag::AnyShader);
				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("NormalRoughness");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteDepthStencil("DepthStencil");

				NewBufferProperties _RenderGrassOnTerrainIndirectArgsProperties{};
				_RenderGrassOnTerrainIndirectArgsProperties.stride = sizeof(IndirectDraw);
				_RenderGrassOnTerrainIndirectArgsProperties.size = 1u * _RenderGrassOnTerrainIndirectArgsProperties.stride;
				_RenderGrassOnTerrainIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_RenderGrassOnTerrainIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("RenderGrassOnTerrainIndirectArgs", _RenderGrassOnTerrainIndirectArgsProperties);
				builder.WriteCopyDstBuffer("RenderGrassOnTerrainIndirectArgs");

				shaderManger.CreateGraphicsShader("RenderGrassOnTerrain",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ObjectOnTerrain/GrassRenderer.hlsl";
						proxy.psFilepath = proxy.vsFilepath;

						proxy.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16_FLOAT,
						};
					});

				commandSignatureManger.CreateCommandSignature("RenderGrassOnTerrain",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// PassDataCB
						proxy.AddIndirectArgument(GHL::IndirectDrawArgument{});						// DrawArgument
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDraw));
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* shadingResult = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* normalRoughness = resourceStorage->GetResourceByName("NormalRoughness")->GetTexture();
				auto* screenVelocity = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
				auto* depthStencil = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();

				auto* grassPlacementBuffer = resourceStorage->GetResourceByName("GrassPlacementBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("RenderGrassOnTerrainIndirectArgs")->GetBuffer();

				renderObjectOnTerrainPassData.grassPlacementBufferIndex = grassPlacementBuffer->GetUADescriptor()->GetHeapIndex();
				renderObjectOnTerrainPassData.numVertices = lod0NumVertices;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(RenderObjectOnTerrainPassData));
				memcpy(passDataAlloc.cpuAddress, &renderObjectOnTerrainPassData, sizeof(RenderObjectOnTerrainPassData));

				IndirectDraw indirectDraw{};
				indirectDraw.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDraw.passDataAddress = passDataAlloc.gpuAddress;
				indirectDraw.drawArguments.VertexCountPerInstance = lod0NumVertices;
				indirectDraw.drawArguments.InstanceCount = 0u;
				indirectDraw.drawArguments.StartVertexLocation = 0u;
				indirectDraw.drawArguments.StartInstanceLocation = 0u;

				// 更新indirectArgs特别是其中的InstanceCount
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				// barrierBatch =  commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassPlacementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDraw, sizeof(IndirectDraw));
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u + sizeof(UINT),
					grassPlacementBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(grassPlacementBuffer, GHL::EResourceState::AnyShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				auto& shadingResultDesc = shadingResult->GetResourceFormat().GetTextureDesc();
				uint16_t width = static_cast<uint16_t>(shadingResultDesc.width);
				uint16_t height = static_cast<uint16_t>(shadingResultDesc.height);
				commandBuffer.SetRenderTargets(
					{
						shadingResult,
						normalRoughness,
						screenVelocity,
					},
					depthStencil);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("RenderGrassOnTerrain");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("RenderGrassOnTerrain", indirectArgs, 1u);
			}
			);
	}

}