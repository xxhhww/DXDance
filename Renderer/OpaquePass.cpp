#include "Renderer/OpaquePass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandSignatureManger.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"

#include <atomic>

namespace Renderer {

	void OpaquePass::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		uint32_t maxOpaqueItems = 1024u;

		renderGraph.AddPass(
			"OpaqueCullingPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _CulledDeferredItemIndirectArgsProperties{};
				_CulledDeferredItemIndirectArgsProperties.stride = sizeof(GpuItemIndirectDrawIndexedData);
				_CulledDeferredItemIndirectArgsProperties.size = maxOpaqueItems * _CulledDeferredItemIndirectArgsProperties.stride;
				_CulledDeferredItemIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer | GHL::EBufferMiscFlag::IndirectArgs;
				_CulledDeferredItemIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("CulledDeferredItemIndirectArgs", _CulledDeferredItemIndirectArgsProperties);
				builder.WriteBuffer("CulledDeferredItemIndirectArgs");

				shaderManger.CreateComputeShader("OpaqueCullingPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Opaque/OpaqueCullingPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* frameTracker = renderContext.frameTracker;

				auto* deferredItemDataBuffer					= resourceStorage->GetResourceByName("DeferredItemDataBuffer")->GetBuffer();
				auto* deferredItemIndirectDrawIndexedDataBuffer = resourceStorage->GetResourceByName("DeferredItemIndirectDrawIndexedDataBuffer")->GetBuffer();
				auto* culledDeferredItemIndirectArgs			= resourceStorage->GetResourceByName("CulledDeferredItemIndirectArgs")->GetBuffer();

				mGpuCullingPassData.deferredItemDataBufferIndex = deferredItemDataBuffer->GetSRDescriptor()->GetHeapIndex();
				mGpuCullingPassData.deferredItemIndirectDrawIndexedDataBufferIndex = deferredItemIndirectDrawIndexedDataBuffer->GetSRDescriptor()->GetHeapIndex();
				mGpuCullingPassData.culledDeferredItemIndirectArgsIndex = culledDeferredItemIndirectArgs->GetUADescriptor()->GetHeapIndex();
				mGpuCullingPassData.itemNumsPerFrame = resourceStorage->rootItemNumsPerFrame;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GpuCullingPassData));
				memcpy(passDataAlloc.cpuAddress, &mGpuCullingPassData, sizeof(GpuCullingPassData));

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(deferredItemDataBuffer, GHL::EResourceState::NonPixelShaderAccess);
					barrierBatch += commandBuffer.TransitionImmediately(deferredItemIndirectDrawIndexedDataBuffer, GHL::EResourceState::NonPixelShaderAccess);
					barrierBatch += commandBuffer.TransitionImmediately(culledDeferredItemIndirectArgs, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(culledDeferredItemIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}
				commandBuffer.ClearCounterBuffer(culledDeferredItemIndirectArgs, 0u);
				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(culledDeferredItemIndirectArgs->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				uint32_t threadGroupCountX = (resourceStorage->rootItemNumsPerFrame + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("OpaqueCullingPass");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, 1u, 1u);
			}
		);

		renderGraph.AddPass(
			"OpaquePass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("CulledDeferredItemIndirectArgs", Renderer::ShaderAccessFlag::NonPixelShader);

				NewTextureProperties _GBufferAlbedoMetalnessProperties{};
				_GBufferAlbedoMetalnessProperties.width = finalOutputDesc.width;
				_GBufferAlbedoMetalnessProperties.height = finalOutputDesc.height;
				_GBufferAlbedoMetalnessProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				_GBufferAlbedoMetalnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferAlbedoMetalness", _GBufferAlbedoMetalnessProperties);
				builder.WriteRenderTarget("GBufferAlbedoMetalness");

				NewTextureProperties _GBufferPositionEmissionProperties{};
				_GBufferPositionEmissionProperties.width = finalOutputDesc.width;
				_GBufferPositionEmissionProperties.height = finalOutputDesc.height;
				_GBufferPositionEmissionProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferPositionEmissionProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferPositionEmission", _GBufferPositionEmissionProperties);
				builder.WriteRenderTarget("GBufferPositionEmission");

				NewTextureProperties _GBufferNormalRoughnessProperties{};
				_GBufferNormalRoughnessProperties.width = finalOutputDesc.width;
				_GBufferNormalRoughnessProperties.height = finalOutputDesc.height;
				_GBufferNormalRoughnessProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferNormalRoughnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferNormalRoughness", _GBufferNormalRoughnessProperties);
				builder.WriteRenderTarget("GBufferNormalRoughness");

				NewTextureProperties _GBufferMotionVectorProperties{};
				_GBufferMotionVectorProperties.width = finalOutputDesc.width;
				_GBufferMotionVectorProperties.height = finalOutputDesc.height;
				_GBufferMotionVectorProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferMotionVectorProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferMotionVector", _GBufferMotionVectorProperties);
				builder.WriteRenderTarget("GBufferMotionVector");

				NewTextureProperties _GBufferViewDepthProperties{};
				_GBufferViewDepthProperties.width = finalOutputDesc.width;
				_GBufferViewDepthProperties.height = finalOutputDesc.height;
				_GBufferViewDepthProperties.format = DXGI_FORMAT_R32_FLOAT;
				_GBufferViewDepthProperties.clearValue = GHL::ColorClearValue{ std::numeric_limits<float>::max() };
				builder.DeclareTexture("GBufferViewDepth", _GBufferViewDepthProperties);
				builder.WriteRenderTarget("GBufferViewDepth");

				NewTextureProperties _GBufferDepthStencilProperties{};
				_GBufferDepthStencilProperties.width = finalOutputDesc.width;
				_GBufferDepthStencilProperties.height = finalOutputDesc.height;
				_GBufferDepthStencilProperties.format = DXGI_FORMAT_D32_FLOAT;
				_GBufferDepthStencilProperties.clearValue = GHL::DepthStencilClearValue{ 1.0f, 0u };
				builder.DeclareTexture("GBufferDepthStencil", _GBufferDepthStencilProperties);
				builder.WriteDepthStencil("GBufferDepthStencil");

				shaderManger.CreateGraphicsShader("OpaquePass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Opaque/OpaquePass.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R8G8B8A8_UNORM,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R32_FLOAT
						};
					});

				commandSignatureManger.CreateCommandSignature("OpaquePass",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// passDataCB
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });	// LightDataSB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 3u });	// ItemDataCB
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});			// VertexBuffer
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});			// IndexBuffer
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});				// DrawIndexedArgument
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(GpuItemIndirectDrawIndexedData));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* deferredItemIndirectArgs = resourceStorage->GetResourceByName("CulledDeferredItemIndirectArgs")->GetBuffer();
				auto* gBufferAlbedoMetalness   = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission  = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness   = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector      = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth         = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil      = resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();

				commandBuffer.ClearRenderTarget(gBufferAlbedoMetalness);
				commandBuffer.ClearRenderTarget(gBufferPositionEmission);
				commandBuffer.ClearRenderTarget(gBufferNormalRoughness);
				commandBuffer.ClearRenderTarget(gBufferMotionVector);
				commandBuffer.ClearRenderTarget(gBufferViewDepth);
				commandBuffer.ClearDepth(gBufferDepthStencil, 1.0f);
				commandBuffer.SetRenderTargets(
					{
						gBufferAlbedoMetalness,
						gBufferPositionEmission,
						gBufferNormalRoughness,
						gBufferMotionVector,
						gBufferViewDepth
					},
					gBufferDepthStencil);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(deferredItemIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(deferredItemIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				uint16_t width = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);

				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("OpaquePass");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("OpaquePass", deferredItemIndirectArgs, maxOpaqueItems);
			});
	}

}