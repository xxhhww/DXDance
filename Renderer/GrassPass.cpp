#include "Renderer/GrassPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	struct IndirectDispatch {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_DISPATCH_ARGUMENTS  dispatchArguments;
		uint32_t pad1;
	};

	struct IndirectDrawIndexed {
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	void GrassPass::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		// 计算安置点的Y轴坐标
		renderGraph.AddPass(
			"GrassBladeGenerator",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _GrassBladeBufferProperties{};
				_GrassBladeBufferProperties.stride = sizeof(GrassBlade);
				_GrassBladeBufferProperties.size = smMaxGrassBladeSize * _GrassBladeBufferProperties.stride;
				_GrassBladeBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("GrassBladeBuffer", _GrassBladeBufferProperties);
				builder.WriteBuffer("GrassBladeBuffer");

				NewBufferProperties _GrassBladeGeneratorIndirectArgsProperties{};
				_GrassBladeGeneratorIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_GrassBladeGeneratorIndirectArgsProperties.size = 1u * _GrassBladeGeneratorIndirectArgsProperties.stride;
				_GrassBladeGeneratorIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_GrassBladeGeneratorIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("GrassBladeGeneratorIndirectArgs", _GrassBladeGeneratorIndirectArgsProperties);
				builder.WriteCopyDstBuffer("GrassBladeGeneratorIndirectArgs");

				shaderManger.CreateComputeShader("GrassBladeGenerator",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/GrassBladeGenerator.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("GrassBladeGenerator",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectDispatchArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDispatch));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* grassBladeBuffer = resourceStorage->GetResourceByName("GrassBladeBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("GrassBladeGeneratorIndirectArgs")->GetBuffer();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* terrainNormalMap = resourceStorage->GetResourceByName("TerrainNormalMap")->GetTexture();

				grassGeneratorPassData.grassBladeBufferIndex0 = grassBladeBuffer->GetUADescriptor()->GetHeapIndex();
				grassGeneratorPassData.terrainHeightMapIndex = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.terrainNormalMapIndex = terrainNormalMap->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.grassBladeSizePerAxis = smGrassBladeSizePerAxis;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GrassGeneratorPassData));
				memcpy(passDataAlloc.cpuAddress, &grassGeneratorPassData, sizeof(GrassGeneratorPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = (smGrassBladeSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountY = (smGrassBladeSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				// 更新grassBladeBuffer与indirectArgs
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.ClearCounterBuffer(grassBladeBuffer, 0u);

				barrierBatch = commandBuffer.TransitionImmediately(grassBladeBuffer, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("GrassBladeGenerator");
				commandBuffer.ExecuteIndirect("GrassBladeGenerator", indirectArgs, 1u);
			}
			);

		renderGraph.AddPass(
			"GrassRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("GrassBladeBuffer", ShaderAccessFlag::AnyShader);
				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("NormalRoughness");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteDepthStencil("DepthStencil");

				NewBufferProperties _GrassRendererIndirectArgsProperties{};
				_GrassRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_GrassRendererIndirectArgsProperties.size = 1u * _GrassRendererIndirectArgsProperties.stride;
				_GrassRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_GrassRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("GrassRendererIndirectArgs", _GrassRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("GrassRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("GrassRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/GrassRenderer.hlsl";
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

				commandSignatureManger.CreateCommandSignature("GrassRenderer",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// PassDataCB
						proxy.AddIndirectArgument(GHL::IndirectDrawArgument{});						// DrawArgument
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* shadingResult = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* normalRoughness = resourceStorage->GetResourceByName("NormalRoughness")->GetTexture();
				auto* screenVelocity = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
				auto* depthStencil = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();

				auto* grassBladeBuffer = resourceStorage->GetResourceByName("GrassBladeBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("GrassRendererIndirectArgs")->GetBuffer();

				grassRendererPassData.grassBladeBufferIndex0 = grassBladeBuffer->GetUADescriptor()->GetHeapIndex();
				grassRendererPassData.grassBladeBufferIndex1;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GrassRendererPassData));
				memcpy(passDataAlloc.cpuAddress, &grassRendererPassData, sizeof(GrassRendererPassData));

				IndirectDrawIndexed indirectDrawIndexed{};
				indirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexed.passDataAddress = passDataAlloc.gpuAddress;
				indirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = grassMesh->GetIndexCount();
				indirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

				// 更新indirectArgs特别是其中的InstanceCount
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				// barrierBatch =  commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDrawIndexed, sizeof(IndirectDrawIndexed));
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u + sizeof(UINT),
					grassBladeBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer, GHL::EResourceState::AnyShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				uint16_t width = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);
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
				commandBuffer.SetGraphicsPipelineState("GrassRenderer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("GrassRenderer", indirectArgs, 1u);
			}
			);
	}

	void GrassPass::InitializePass(RenderEngine* renderEngine) {

	}

}