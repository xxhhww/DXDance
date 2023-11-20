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

	#pragma pack (1)
	struct IndirectDraw {
	public:
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS lightDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS perItemDataAddress;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	struct ItemData {
	public:
		Math::Matrix4 prevModelTrans;		// 前一帧的世界变换矩阵
		Math::Matrix4 currModelTrans;		// 当前帧的世界变换矩阵
		Math::Vector4 center;				// boundingBox
		Math::Vector4 extend;
	};

	void OpaquePass::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		uint32_t maxOpaqueItems = 300.0f;
		renderGraph.AddPass(
			"GpuCullingPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _OpaqueItemIndirectArgsProperties{};
				_OpaqueItemIndirectArgsProperties.stride = sizeof(IndirectDraw);
				_OpaqueItemIndirectArgsProperties.size = maxOpaqueItems * _OpaqueItemIndirectArgsProperties.stride;
				_OpaqueItemIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_OpaqueItemIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("OpaqueItemIndirectArgs", _OpaqueItemIndirectArgsProperties);
				builder.WriteCopyDstBuffer("OpaqueItemIndirectArgs");

				NewBufferProperties _OpaqueItemDataArrayProperties{};
				_OpaqueItemDataArrayProperties.stride = sizeof(ItemData);
				_OpaqueItemDataArrayProperties.size = maxOpaqueItems * _OpaqueItemDataArrayProperties.stride;
				_OpaqueItemDataArrayProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_OpaqueItemDataArrayProperties.aliased = false;
				builder.DeclareBuffer("OpaqueItemDataArray", _OpaqueItemDataArrayProperties);
				builder.WriteCopyDstBuffer("OpaqueItemDataArray");

				shaderManger.CreateComputeShader("GpuCullingPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Opaque/GpuCullingPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* frameTracker = renderContext.frameTracker;

				auto* opaqueItemIndirectArgs = resourceStorage->GetResourceByName("OpaqueItemIndirectArgs")->GetBuffer();
				auto* opaqueItemDataArray = resourceStorage->GetResourceByName("OpaqueItemDataArray")->GetBuffer();

				std::vector<IndirectDraw> indirectCommandArgs(maxOpaqueItems, IndirectDraw{});
				std::vector<ItemData> itemDataArrays(maxOpaqueItems, ItemData{});
				std::atomic<int32_t> atomicIndex = -1;
				ECS::Entity::Foreach([&](ECS::Transform& transform, ECS::MeshRenderer& meshRenderer) {
					int32_t index = ++atomicIndex;

					IndirectDraw indirectDraw{};
					indirectDraw.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
					indirectDraw.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
					indirectDraw.perItemDataAddress = opaqueItemDataArray->GetGpuAddress() + index * sizeof(ItemData);
					indirectDraw.vertexBufferView = meshRenderer.mesh->GetVertexBuffer()->GetVBDescriptor();
					indirectDraw.indexBufferView = meshRenderer.mesh->GetIndexBuffer()->GetIBDescriptor();
					indirectDraw.drawIndexedArguments.BaseVertexLocation = 0u;
					indirectDraw.drawIndexedArguments.StartInstanceLocation = 0u;
					indirectDraw.drawIndexedArguments.InstanceCount = 1u;
					indirectDraw.drawIndexedArguments.StartIndexLocation = 0u;
					indirectDraw.drawIndexedArguments.IndexCountPerInstance = meshRenderer.mesh->GetIndexCount();

					ItemData itemData{};
					Math::Matrix4 worldMatrix = transform.GetWorldMatrix().Transpose();
					itemData.prevModelTrans = frameTracker->IsFirstFrame() ? worldMatrix : itemData.currModelTrans;
					itemData.currModelTrans = worldMatrix;

					indirectCommandArgs.at(index) = std::move(indirectDraw);
					itemDataArrays.at(index) = std::move(itemData);
				});
				uint32_t validItemSize = atomicIndex + 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(opaqueItemIndirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(opaqueItemDataArray, GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);
				commandBuffer.UploadBufferRegion(opaqueItemIndirectArgs, 0u, indirectCommandArgs.data(), validItemSize * sizeof(IndirectDraw));
				commandBuffer.ClearCounterBuffer(opaqueItemIndirectArgs, validItemSize);
				commandBuffer.UploadBufferRegion(opaqueItemDataArray, 0u, itemDataArrays.data(), validItemSize * sizeof(ItemData));
				commandBuffer.ClearCounterBuffer(opaqueItemDataArray, validItemSize);
			}
		);

		renderGraph.AddPass(
			"OpaquePass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("OpaqueItemIndirectArgs", Renderer::ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("OpaqueItemDataArray", Renderer::ShaderAccessFlag::NonPixelShader);

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
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });		// FrameDataCB
						// proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// passDataAlloc不使用
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });		// LightDataSB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 3u });		// PerItemCB
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});					// DrawIndexedArgument
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDraw));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* opaqueItemIndirectArgs  = resourceStorage->GetResourceByName("OpaqueItemIndirectArgs")->GetBuffer();
				auto* opaqueItemDataArray     = resourceStorage->GetResourceByName("OpaqueItemDataArray")->GetBuffer();
				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector     = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil     = resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();

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

				uint16_t width = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);

				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("OpaquePass");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("OpaquePass", opaqueItemIndirectArgs, maxOpaqueItems);
			});
	}

}