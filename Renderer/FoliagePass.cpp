#include "Renderer/FoliagePass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/FixedTextureHelper.h"

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
		D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW      indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	void FoliagePass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		// 计算安置点的Y轴坐标
		renderGraph.AddPass(
			"FoliagePlacer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _FoliagePlacementBufferProperties{};
				_FoliagePlacementBufferProperties.stride = sizeof(Placement);
				_FoliagePlacementBufferProperties.size = smMaxPlacementSize * _FoliagePlacementBufferProperties.stride;
				_FoliagePlacementBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("FoliagePlacementBuffer", _FoliagePlacementBufferProperties);
				builder.WriteBuffer("FoliagePlacementBuffer");

				NewBufferProperties _FoliagePlacerIndirectArgsProperties{};
				_FoliagePlacerIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_FoliagePlacerIndirectArgsProperties.size = 1u * _FoliagePlacerIndirectArgsProperties.stride;
				_FoliagePlacerIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_FoliagePlacerIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("FoliagePlacerIndirectArgs", _FoliagePlacerIndirectArgsProperties);
				builder.WriteCopyDstBuffer("FoliagePlacerIndirectArgs");

				shaderManger.CreateComputeShader("FoliagePlacer",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Foliage/FoliagePlacer.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("FoliagePlacer",
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
				auto* resourceStorage  = renderContext.resourceStorage;

				auto* placementBuffer  = resourceStorage->GetResourceByName("FoliagePlacementBuffer")->GetBuffer();
				auto* indirectArgs	   = resourceStorage->GetResourceByName("FoliagePlacerIndirectArgs")->GetBuffer();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* terrainNormalMap = resourceStorage->GetResourceByName("TerrainNormalMap")->GetTexture();

				foliagePassData.placementBufferIndex = placementBuffer->GetUADescriptor()->GetHeapIndex();
				foliagePassData.heightMapIndex       = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
				foliagePassData.normalMapIndex       = terrainNormalMap->GetSRDescriptor()->GetHeapIndex();
				foliagePassData.placementSizePerAxis = smPlacementSizePerAxis;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(FoliagePassData));
				memcpy(passDataAlloc.cpuAddress, &foliagePassData, sizeof(FoliagePassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress  = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = (smPlacementSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountY = (smPlacementSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				// 更新placementBuffer与indirectArgs
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.ClearCounterBuffer(placementBuffer, 0u);

				barrierBatch =  commandBuffer.TransitionImmediately(placementBuffer, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("FoliagePlacer");
				commandBuffer.ExecuteIndirect("FoliagePlacer", indirectArgs, 1u);
			}
		);

		renderGraph.AddPass(
			"FoliageRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("FoliagePlacementBuffer", ShaderAccessFlag::AnyShader);

				builder.WriteRenderTarget("GBufferAlbedoMetalness");
				builder.WriteRenderTarget("GBufferPositionEmission");
				builder.WriteRenderTarget("GBufferNormalRoughness");
				builder.WriteRenderTarget("GBufferMotionVector");
				builder.WriteRenderTarget("GBufferViewDepth");
				builder.WriteDepthStencil("GBufferDepthStencil");

				NewBufferProperties _FoliageRendererIndirectArgsProperties{};
				_FoliageRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_FoliageRendererIndirectArgsProperties.size	= 1u * _FoliageRendererIndirectArgsProperties.stride;
				_FoliageRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_FoliageRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("FoliageRendererIndirectArgs", _FoliageRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("FoliageRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("FoliageRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Foliage/FoliageRenderer.hlsl";
						proxy.psFilepath = proxy.vsFilepath;

						proxy.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

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

				commandSignatureManger.CreateCommandSignature("FoliageRenderer",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// PassDataCB
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});			// VertexBuffer
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});			// IndexBuffer
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});				// DrawIndexed
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* gBufferAlbedoMetalness	= resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission	= resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness	= resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector		= resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth			= resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil		= resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();
				auto* placementBuffer			= resourceStorage->GetResourceByName("FoliagePlacementBuffer")->GetBuffer();
				auto* indirectArgs				= resourceStorage->GetResourceByName("FoliageRendererIndirectArgs")->GetBuffer();
				
				foliagePassData.placementBufferIndex = placementBuffer->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(FoliagePassData));
				memcpy(passDataAlloc.cpuAddress, &foliagePassData, sizeof(FoliagePassData));

				IndirectDrawIndexed indirectDrawIndexed{};
				indirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexed.passDataAddress  = passDataAlloc.gpuAddress;
				indirectDrawIndexed.vertexBufferView = foliageMesh->GetVertexBuffer()->GetVBDescriptor();
				indirectDrawIndexed.indexBufferView  = foliageMesh->GetIndexBuffer()->GetIBDescriptor();
				indirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = foliageMesh->GetIndexCount();
				indirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

				// 更新indirectArgs特别是其中的InstanceCount
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				// barrierBatch =  commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDrawIndexed, sizeof(IndirectDrawIndexed));
				commandBuffer.CopyBufferRegion(indirectArgs,
					sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u + sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(UINT),
					placementBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(placementBuffer, GHL::EResourceState::AnyShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				uint16_t width  = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);
				commandBuffer.SetRenderTargets(
					{
						gBufferAlbedoMetalness,
						gBufferPositionEmission,
						gBufferNormalRoughness,
						gBufferMotionVector,
						gBufferViewDepth
					},
					gBufferDepthStencil);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("FoliageRenderer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("FoliageRenderer", indirectArgs, 1u);
			}
			);
	}

	void FoliagePass::InitializePass(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		/*
		static float grassVerts[] = {
			//Position				//1D-Texture-Coordinate		//SwayStrength
			-0.02f, 0.0f,  0.0f,	0.0f,						0.0f,
			 0.02f, 0.0f,  0.0f,	1.0f,						0.0f,
			-0.02f, 0.5f,  0.0f,	0.0f,						0.3f,
			 0.02f, 0.5f,  0.0f,	1.0f,						0.3f,
			-0.02f, 0.8f,  0.0f,	0.0f,						0.6f,
			 0.02f, 0.8f,  0.0f,	1.0f,						0.6f,
			 0.0f,  1.0f,  0.0f,	0.5f,						1.0f
		};

		*/

		// 加载FoliageMesh
		{
			std::vector<Vertex> vertices = {
				{
					{ -0.02f, 0.0f, 0.0f }, 
					{ 0.0f, 0.0f }, 
					{ 0.0f, 0.0f, 0.0f }, 
					{ 0.0f, 0.0f, 0.0f }, 
					{ 0.0f, 0.0f, 0.0f }, 
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f } 
				},
				{
					{ 0.02f, 0.0f, 0.0f },
					{ 1.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f }
				},
				{
					{ -0.02f, 0.5f, 0.0f },
					{ 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.3f, 0.0f, 0.0f, 0.0f }
				},
				{
					{ 0.02f, 0.5f, 0.0f },
					{ 1.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.3f, 0.0f, 0.0f, 0.0f }
				},
				{
					{ -0.02f, 0.8f, 0.0f },
					{ 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.6f, 0.0f, 0.0f, 0.0f }
				},
				{
					{ 0.02f, 0.8f, 0.0f },
					{ 1.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 0.6f, 0.0f, 0.0f, 0.0f }
				},
				{
					{ 0.0f, 1.0f, 0.0f },
					{ 0.5f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 0.0f },
					{ 1.0f, 0.0f, 0.0f, 0.0f }
				},
			};

			std::vector<uint32_t> indices = {
				0, 1, 2,
				1, 3, 2,
				2, 3, 4,
				3, 5, 4,
				4, 5, 6
			};

			Renderer::BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Renderer::Vertex);
			vbDesc.size = vbDesc.stride * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			Renderer::BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = ibDesc.stride * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			foliageMesh = std::make_unique<Renderer::Mesh>(
				device,
				ResourceFormat{ device, vbDesc },
				ResourceFormat{ device, ibDesc },
				nullptr,
				nullptr);

			foliageMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
			foliageMesh->GetVertexBuffer()->SetDebugName("FoliageMeshVertexBuffer");
			foliageMesh->GetIndexBuffer()->SetDebugName("FoliageMeshIndexBuffer");
		}

		// 加载FoliageAlbedoMap
		{
			foliageAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Foliage/Grass_Albedo_1.png");
			resourceStateTracker->StartTracking(foliageAlbedoMap);
		}

		// 加载FoliageNormalMap
		{
			foliageNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Foliage/Grass_Normal_1.png"
			);
			resourceStateTracker->StartTracking(foliageNormalMap);
		}
	}

}