#include "Renderer/GrassPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/FixedTextureHelper.h"

namespace Renderer {

	struct IndirectDraw {
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_DRAW_ARGUMENTS         drawArguments;
	};

	void GrassPass::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"GrassBladeGenerator",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("NearbyNodeList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("NodeDescriptorList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("LODDescriptorList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _CurrentNodeIndexBufferProperties{};	// 当前Dispatch处理的Node索引
				_CurrentNodeIndexBufferProperties.stride = sizeof(uint32_t);
				_CurrentNodeIndexBufferProperties.size = 1u * _CurrentNodeIndexBufferProperties.stride;
				_CurrentNodeIndexBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_CurrentNodeIndexBufferProperties.aliased = false;
				builder.DeclareBuffer("CurrentNodeIndexBuffer", _CurrentNodeIndexBufferProperties);
				builder.WriteBuffer("CurrentNodeIndexBuffer");

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

				auto* nearbyNodeList = resourceStorage->GetResourceByName("NearbyNodeList")->GetBuffer();
				auto* currentNodeIndexBuffer = resourceStorage->GetResourceByName("CurrentNodeIndexBuffer")->GetBuffer();
				auto* grassBladeBuffer = resourceStorage->GetResourceByName("GrassBladeBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("GrassBladeGeneratorIndirectArgs")->GetBuffer();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* terrainNormalMap = resourceStorage->GetResourceByName("TerrainNormalMap")->GetTexture();
				auto* nodeDescriptorList = resourceStorage->GetResourceByName("NodeDescriptorList")->GetBuffer();
				auto* lodDescriptorList = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();

				grassGeneratorPassData.grassBladeBufferIndex0 = grassBladeBuffer->GetUADescriptor()->GetHeapIndex();
				grassGeneratorPassData.terrainHeightMapIndex = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.terrainNormalMapIndex = terrainNormalMap->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.grassBladeSizePerAxisPerTile = smGrassBladeSizePerAxisPerTile;
				grassGeneratorPassData.nearbyNodeListIndex = nearbyNodeList->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.currentNodeIndexBufferIndex = currentNodeIndexBuffer->GetUADescriptor()->GetHeapIndex();
				grassGeneratorPassData.nodeDescriptorListIndex = nodeDescriptorList->GetSRDescriptor()->GetHeapIndex();
				grassGeneratorPassData.lodDescriptorListIndex = lodDescriptorList->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GrassGeneratorPassData));
				memcpy(passDataAlloc.cpuAddress, &grassGeneratorPassData, sizeof(GrassGeneratorPassData));

				/*
				for (auto& indirectDispatch : indirectDispatchs) {
					indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
					indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
					indirectDispatch.dispatchArguments.ThreadGroupCountX = smGroupSize;
					indirectDispatch.dispatchArguments.ThreadGroupCountY = smGroupSize;
					indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;
				}
				*/
				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountY = smGroupSizePerTile;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = smGroupSizePerTile;

				// 更新grassBladeBuffer与indirectArgs
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(currentNodeIndexBuffer, GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				/*
				commandBuffer.UploadBufferRegion(indirectArgs, 0u, indirectDispatchs.data(), indirectDispatchs.size() * sizeof(IndirectDispatch));
				commandBuffer.CopyCounterBuffer(indirectArgs, nearbyNodeList);
				*/
				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, nearbyNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.ClearCounterBuffer(grassBladeBuffer, 0u);
				commandBuffer.ClearBufferWithValue(currentNodeIndexBuffer, 0u);

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(currentNodeIndexBuffer, GHL::EResourceState::UnorderedAccess);
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
				_GrassRendererIndirectArgsProperties.stride = sizeof(IndirectDraw);
				_GrassRendererIndirectArgsProperties.size = 1u * _GrassRendererIndirectArgsProperties.stride;
				_GrassRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_GrassRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("GrassRendererIndirectArgs", _GrassRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("GrassRendererIndirectArgs");

				NewBufferProperties _GrassVerticesBufferProperties{};
				_GrassVerticesBufferProperties.stride = sizeof(Vertex);
				_GrassVerticesBufferProperties.size = grassVertices.size() * _GrassVerticesBufferProperties.stride;
				_GrassVerticesBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassVerticesBufferProperties.aliased = false;
				builder.DeclareBuffer("GrassVerticesBuffer", _GrassVerticesBufferProperties);
				builder.WriteBuffer("GrassVerticesBuffer");

				NewBufferProperties _GrassIndicesBufferProperties{};
				_GrassIndicesBufferProperties.stride = sizeof(uint32_t);
				_GrassIndicesBufferProperties.size = grassIndices.size() * _GrassIndicesBufferProperties.stride;
				_GrassIndicesBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassIndicesBufferProperties.aliased = false;
				builder.DeclareBuffer("GrassIndicesBuffer", _GrassIndicesBufferProperties);
				builder.WriteBuffer("GrassIndicesBuffer");

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
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });	// LightDataSB
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

				auto* grassBladeBuffer = resourceStorage->GetResourceByName("GrassBladeBuffer")->GetBuffer();
				auto* grassVerticesBuffer = resourceStorage->GetResourceByName("GrassVerticesBuffer")->GetBuffer();
				auto* grassIndicesBuffer = resourceStorage->GetResourceByName("GrassIndicesBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("GrassRendererIndirectArgs")->GetBuffer();

				grassRendererPassData.grassBladeBufferIndex0 = grassBladeBuffer->GetUADescriptor()->GetHeapIndex();
				grassRendererPassData.grassBladeBufferIndex1 = grassBladeBuffer->GetUADescriptor()->GetHeapIndex();
				grassRendererPassData.grassMeshVerticesBufferIndex = grassVerticesBuffer->GetUADescriptor()->GetHeapIndex();
				grassRendererPassData.grassMeshIndicesBufferIndex = grassIndicesBuffer->GetUADescriptor()->GetHeapIndex();
				grassRendererPassData.grassAlbedoMapIndex = grassAlbedoMap->GetSRDescriptor()->GetHeapIndex();
				grassRendererPassData.grassNormalMapIndex = grassNormalMap->GetSRDescriptor()->GetHeapIndex();

				if (!isInitialized) {
					auto barrierBatch = GHL::ResourceBarrierBatch{};

					barrierBatch = commandBuffer.TransitionImmediately(grassVerticesBuffer, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(grassIndicesBuffer, GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.UploadBufferRegion(grassVerticesBuffer, 0u,
						grassVertices.data(), grassVertices.size() * sizeof(Vertex)
					);
					commandBuffer.UploadBufferRegion(grassIndicesBuffer, 0u,
						grassIndices.data(), grassIndices.size() * sizeof(uint32_t)
					);

					isInitialized = true;
				}

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GrassRendererPassData));
				memcpy(passDataAlloc.cpuAddress, &grassRendererPassData, sizeof(GrassRendererPassData));

				IndirectDraw indirectDraw{};
				indirectDraw.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDraw.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				indirectDraw.passDataAddress = passDataAlloc.gpuAddress;
				indirectDraw.drawArguments.VertexCountPerInstance = grassIndices.size();
				indirectDraw.drawArguments.InstanceCount = 0u;
				indirectDraw.drawArguments.StartVertexLocation = 0u;
				indirectDraw.drawArguments.StartInstanceLocation = 0u;

				// 更新indirectArgs特别是其中的InstanceCount
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				// barrierBatch =  commandBuffer.TransitionImmediately(placementBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassBladeBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDraw, sizeof(IndirectDraw));
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(UINT),
					grassBladeBuffer->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
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

		{
			// vertex 0
			grassVertices.emplace_back(
				Math::Vector3{ 0.00f, 0.78f, 0.17f },
				Math::Vector2{ 0.45f, 0.22f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.309f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 1
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 0.0f, -0.17f },
				Math::Vector2{ 0.55f, 0.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.0f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 2
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 0.0f, 0.17f },
				Math::Vector2{ 0.45f, 0.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }
			);

			// vertex 3
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 0.78f, -0.17f },
				Math::Vector2{ 0.55f, 0.22f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.309f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 4
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 1.36f, -0.16f },
				Math::Vector2{ 0.55f, 0.38f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.472f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 5
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 1.36f, 0.16f },
				Math::Vector2{ 0.45f, 0.38f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.472f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 6
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 1.91f, -0.15f },
				Math::Vector2{ 0.54f, 0.54f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.601f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 7
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 1.91f, 0.15f },
				Math::Vector2{ 0.46f, 0.54f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.601f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 8
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 2.37f, -0.13f },
				Math::Vector2{ 0.54f, 0.67f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.716f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 9
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 2.37f, 0.13f },
				Math::Vector2{ 0.46f, 0.67f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.716f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 10
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 2.78f, -0.12f },
				Math::Vector2{ 0.53f, 0.78f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.817f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 11
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 2.78f, 0.12f },
				Math::Vector2{ 0.47f, 0.78f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.817f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 12
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 3.15f, -0.09f },
				Math::Vector2{ 0.53f, 0.89f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.913f, 1.0f, 0.0f, 1.0f }
			);

			// vertex 13
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 3.15f, 0.09f },
				Math::Vector2{ 0.48f, 0.89f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 0.913f, 0.0f, 0.0f, 1.0f }
			);

			// vertex 14
			grassVertices.emplace_back(
				Math::Vector3{ 0.0f, 3.54f, 0.0f },
				Math::Vector2{ 0.5f, 1.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{},
				Math::Vector4{ 1.0f, 0.498f, 0.0f, 1.0f }
			);

			grassIndices = {
				0,  1,  2,
				0,  3,  1,
				0,  4,  3,
				0,  5,  4,
				5,  6,  4,
				5,  7,  6,
				7,  8,  6,
				7,  9,  8,
				9,  10, 8,
				9,  11, 10,
				12, 10, 11,
				11, 13, 12,
				13, 14, 12
			};
		}

		auto* device = renderEngine->mDevice.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		{
			// 加载GrassAlbedoMap
				grassAlbedoMap = FixedTextureHelper::LoadFromFile(
					device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
					"E:/MyProject/DXDance/Resources/Textures/Grass/Grass_Albedo.png");
				resourceStateTracker->StartTracking(grassAlbedoMap);

			// 加载GrassNormalMap
				grassNormalMap = FixedTextureHelper::LoadFromFile(
					device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
					"E:/MyProject/DXDance/Resources/Textures/Grass/Grass_Normal.png"
				);
				resourceStateTracker->StartTracking(grassNormalMap);
		}

		// 填充IndirectDispatchs
		{
			indirectDispatchs.resize(smMaxNodeListSize);
		}
	}

}