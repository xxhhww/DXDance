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

	void OpaquePass::AddPreDepthPass(RenderGraph& renderGraph) {
	}

	void OpaquePass::AddShadowPass(RenderGraph& renderGraph) {
	}

	void OpaquePass::AddForwardPlusPass(RenderGraph& renderGraph) {
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

				NewTextureProperties _ShadingResultProperties{};
				_ShadingResultProperties.width = finalOutputDesc.width;
				_ShadingResultProperties.height = finalOutputDesc.height;
				_ShadingResultProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_ShadingResultProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				_ShadingResultProperties.aliased = false;
				builder.DeclareTexture("ShadingResult", _ShadingResultProperties);
				builder.WriteRenderTarget("ShadingResult");

				NewTextureProperties _NormalRoughnessProperties{};
				_NormalRoughnessProperties.width = finalOutputDesc.width;
				_NormalRoughnessProperties.height = finalOutputDesc.height;
				_NormalRoughnessProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_NormalRoughnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				_NormalRoughnessProperties.aliased = false;
				builder.DeclareTexture("NormalRoughness", _NormalRoughnessProperties);
				builder.WriteRenderTarget("NormalRoughness");

				NewTextureProperties _ScreenVelocityProperties{};
				_ScreenVelocityProperties.width = finalOutputDesc.width;
				_ScreenVelocityProperties.height = finalOutputDesc.height;
				_ScreenVelocityProperties.format = DXGI_FORMAT_R16G16_FLOAT;
				_ScreenVelocityProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				_ScreenVelocityProperties.aliased = false;
				builder.DeclareTexture("ScreenVelocity", _ScreenVelocityProperties);
				builder.WriteRenderTarget("ScreenVelocity");

				NewTextureProperties _DepthStencilProperties{};
				_DepthStencilProperties.width = finalOutputDesc.width;
				_DepthStencilProperties.height = finalOutputDesc.height;
				_DepthStencilProperties.format = DXGI_FORMAT_D32_FLOAT;
				_DepthStencilProperties.clearValue = GHL::DepthStencilClearValue{ 1.0f, 0u };
				// _DepthStencilProperties.aliased = false;
				builder.DeclareTexture("DepthStencil", _DepthStencilProperties);
				builder.WriteDepthStencil("DepthStencil");

				shaderManger.CreateGraphicsShader("OpaquePass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Opaque/OpaquePass.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16_FLOAT,
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

				auto* opaqueItemIndirectArgs = resourceStorage->GetResourceByName("OpaqueItemIndirectArgs")->GetBuffer();
				auto* opaqueItemDataArray = resourceStorage->GetResourceByName("OpaqueItemDataArray")->GetBuffer();
				auto* shadingResult   = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* normalRoughness = resourceStorage->GetResourceByName("NormalRoughness")->GetTexture();
				auto* screenVelocity  = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
				auto* depthStencil    = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();

				commandBuffer.ClearRenderTarget(shadingResult);
				commandBuffer.ClearRenderTarget(normalRoughness);
				commandBuffer.ClearRenderTarget(screenVelocity);
				commandBuffer.ClearDepth(depthStencil, 1.0f);
				commandBuffer.SetRenderTargets(
					{
						shadingResult,
						normalRoughness,
						screenVelocity,
					},
					depthStencil);

				auto& shadingResultDesc = shadingResult->GetResourceFormat().GetTextureDesc();
				uint16_t width = static_cast<uint16_t>(shadingResultDesc.width);
				uint16_t height = static_cast<uint16_t>(shadingResultDesc.height);

				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("OpaquePass");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("OpaquePass", opaqueItemIndirectArgs, maxOpaqueItems);
			});
	}

	void OpaquePass::AddGBufferPass(RenderGraph& renderGraph) {

	}

}