#include "Renderer/TerrainPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

#include "Renderer/FormatConverter.h"

#include "DirectXTex/DirectXTex.h"

#include "Renderer/FixedTextureHelper.h"

#include "GHL/Box.h"

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

	void TerrainPass::AddPass(RenderGraph& renderGraph) {

		uint32_t _MaxNodeListSize = 200u;
		uint32_t _TmpNodeListSize = 50u;

		UpdateNodeAndLodDescriptorArray();

		renderGraph.AddPass(
			"TraverseQuadTree",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _TraverseQuadTreeIndirectArgsProperties;
				_TraverseQuadTreeIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_TraverseQuadTreeIndirectArgsProperties.size = sizeof(IndirectDispatch);
				_TraverseQuadTreeIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_TraverseQuadTreeIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("TraverseQuadTreeIndirectArgs", _TraverseQuadTreeIndirectArgsProperties);
				builder.WriteCopyDstBuffer("TraverseQuadTreeIndirectArgs");

				NewBufferProperties _ConsumeNodeListProperties;
				_ConsumeNodeListProperties.stride = sizeof(uint32_t) * 2u; // uint2
				_ConsumeNodeListProperties.size = _TmpNodeListSize * _ConsumeNodeListProperties.stride;
				_ConsumeNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("ConsumeNodeList", _ConsumeNodeListProperties);
				builder.WriteBuffer("ConsumeNodeList");

				NewBufferProperties _AppendNodeListProperties;
				_AppendNodeListProperties.stride = sizeof(uint32_t) * 2u; // uint2
				_AppendNodeListProperties.size = _TmpNodeListSize * _AppendNodeListProperties.stride;
				_AppendNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("AppendNodeList", _AppendNodeListProperties);
				builder.WriteBuffer("AppendNodeList");

				NewBufferProperties _FinalNodeListProperties;
				_FinalNodeListProperties.stride = sizeof(uint32_t) * 3u; // uint3
				_FinalNodeListProperties.size = _MaxNodeListSize * _FinalNodeListProperties.stride;
				_FinalNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("FinalNodeList", _FinalNodeListProperties);
				builder.WriteBuffer("FinalNodeList");

				NewBufferProperties _NodeDescriptorListProperties;
				_NodeDescriptorListProperties.stride = sizeof(NodeDescriptor);
				_NodeDescriptorListProperties.size = nodeDescriptors.size() * _NodeDescriptorListProperties.stride;
				_NodeDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("NodeDescriptorList", _NodeDescriptorListProperties);
				builder.WriteBuffer("NodeDescriptorList");

				NewBufferProperties _LODDescriptorListProperties;
				_LODDescriptorListProperties.stride = sizeof(LODDescriptor);
				_LODDescriptorListProperties.size = lodDescriptors.size() * _LODDescriptorListProperties.stride;
				_LODDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("LODDescriptorList", _LODDescriptorListProperties);
				builder.WriteBuffer("LODDescriptorList");

				shaderManger.CreateComputeShader("TraverseQuadTree",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainBuilder.hlsl";
						proxy.csEntryPoint = "TraverseQuadTree";
					});

				commandSignatureManger.CreateCommandSignature("TraverseQuadTree",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectDispatchArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDispatch));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator       = renderContext.dynamicAllocator;
				auto* resourceStorage        = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* consumeNodeList    = resourceStorage->GetResourceByName("ConsumeNodeList")->GetBuffer();
				auto* appendNodeList     = resourceStorage->GetResourceByName("AppendNodeList")->GetBuffer();
				auto* finalNodeList      = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* nodeDescriptorList = resourceStorage->GetResourceByName("NodeDescriptorList")->GetBuffer();
				auto* lodDescriptorList  = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();
				auto* indirectArgs       = resourceStorage->GetResourceByName("TraverseQuadTreeIndirectArgs")->GetBuffer();
				
				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("TraverseQuadTree");

				terrainBuilderPassData.worldMeterSize = worldMeterSize;
				terrainBuilderPassData.heightScale = worldHeightScale;
				terrainBuilderPassData.currPassLOD = maxLOD;
				terrainBuilderPassData.consumeNodeListIndex    = consumeNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.appendNodeListIndex     = appendNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.finalNodeListIndex      = finalNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.nodeDescriptorListIndex = nodeDescriptorList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.lodDescriptorListIndex  = lodDescriptorList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.minmaxHeightMapIndex	   = minmaxHeightMap->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::TerrainBuilderPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainPass::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress  = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = maxLODNodeList.size();
				indirectDispatch.dispatchArguments.ThreadGroupCountY = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};

				if (!isInitialized) {
					barrierBatch =  commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.UploadBufferRegion(nodeDescriptorList, 0u, 
						nodeDescriptors.data(), nodeDescriptors.size() * sizeof(TerrainPass::NodeDescriptor)
					);
					commandBuffer.UploadBufferRegion(lodDescriptorList, 0u,
						lodDescriptors.data(), lodDescriptors.size() * sizeof(TerrainPass::LODDescriptor)
					);

					isInitialized = true;
				}

				
				barrierBatch =  commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(minmaxHeightMap.Get(), GHL::EResourceState::NonPixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.UploadBufferRegion(consumeNodeList, 0u,
					maxLODNodeList.data(), maxLODNodeList.size() * sizeof(TerrainPass::NodeLocation)
				);
				commandBuffer.ClearCounterBuffer(consumeNodeList, maxLODNodeList.size());

				commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
				commandBuffer.ClearCounterBuffer(finalNodeList, 0u);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TraverseQuadTree");
				for (int32_t lod = maxLOD; lod >= 0; lod--) {
					if (lod != maxLOD) {
						// 更新TerrainBuilderPassData
						uint32_t tempValue = terrainBuilderPassData.consumeNodeListIndex;
						terrainBuilderPassData.consumeNodeListIndex = terrainBuilderPassData.appendNodeListIndex;
						terrainBuilderPassData.appendNodeListIndex  = tempValue;
						terrainBuilderPassData.currPassLOD = lod;

						passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::TerrainBuilderPassData), 256u);
						memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainPass::TerrainBuilderPassData));

						// 更新IndirectArgs
						barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
						barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
						barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
						commandBuffer.FlushResourceBarrier(barrierBatch);

						commandBuffer.UploadBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 1u, &passDataAlloc.gpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, consumeNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));
						commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
					}

					// ExecuteIndirect
					barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(appendNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(finalNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.ExecuteIndirect("TraverseQuadTree", indirectArgs, 1u);
				
					Buffer* tempBuffer = consumeNodeList;
					consumeNodeList = appendNodeList;
					appendNodeList = tempBuffer;
				}
			}
		);

		renderGraph.AddPass(
			"BuildPatches",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("LODDescriptorList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("FinalNodeList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _CulledPatchListProperties{};
				_CulledPatchListProperties.stride = sizeof(TerrainPass::RenderPatch);
				_CulledPatchListProperties.size = _MaxNodeListSize * _CulledPatchListProperties.stride * 64;
				_CulledPatchListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("CulledPatchList", _CulledPatchListProperties);
				builder.WriteBuffer("CulledPatchList");

				NewBufferProperties _BuildPatchesIndirectArgsProperties{};
				_BuildPatchesIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_BuildPatchesIndirectArgsProperties.size = sizeof(IndirectDispatch);
				_BuildPatchesIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_BuildPatchesIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("BuildPatchesIndirectArgs", _BuildPatchesIndirectArgsProperties);
				builder.WriteCopyDstBuffer("BuildPatchesIndirectArgs");

				shaderManger.CreateComputeShader("BuildPatches",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainBuilder.hlsl";
						proxy.csEntryPoint = "BuildPatches";
					});

				commandSignatureManger.CreateCommandSignature("BuildPatches",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectDispatchArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDispatch));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator       = renderContext.dynamicAllocator;
				auto* resourceStorage        = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* lodDescriptorList = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();
				auto* finalNodeList     = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* culledPatchList   = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs      = resourceStorage->GetResourceByName("BuildPatchesIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("BuildPatches");

				terrainBuilderPassData.worldMeterSize = worldMeterSize;
				terrainBuilderPassData.heightScale = worldHeightScale;
				terrainBuilderPassData.lodDescriptorListIndex = lodDescriptorList->GetSRDescriptor()->GetHeapIndex();
				terrainBuilderPassData.finalNodeListIndex     = finalNodeList->GetSRDescriptor()->GetHeapIndex();
				terrainBuilderPassData.culledPatchListIndex   = culledPatchList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::TerrainBuilderPassData));
				memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainPass::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountY = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, finalNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				commandBuffer.ClearCounterBuffer(culledPatchList, 0u);

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("BuildPatches");
				commandBuffer.ExecuteIndirect("BuildPatches", indirectArgs, 1u);
			}
		);

		renderGraph.AddPass(
			"TerrainRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("GBufferAlbedoMetalness");
				builder.WriteRenderTarget("GBufferPositionEmission");
				builder.WriteRenderTarget("GBufferNormalRoughness");
				builder.WriteRenderTarget("GBufferMotionVector");
				builder.WriteRenderTarget("GBufferViewDepth");
				builder.WriteDepthStencil("GBufferDepthStencil");
				builder.ReadBuffer("CulledPatchList", ShaderAccessFlag::PixelShader);

				NewBufferProperties _TerrainRendererIndirectArgsProperties{};
				_TerrainRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.size = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_TerrainRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("TerrainRendererIndirectArgs", _TerrainRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("TerrainRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("TerrainRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainRenderer.hlsl";
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

				commandSignatureManger.CreateCommandSignature("TerrainRenderer",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector     = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil     = resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();
				auto* culledPatchList         = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs            = resourceStorage->GetResourceByName("TerrainRendererIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("TerrainRenderer");

				terrainRendererPassData.worldMeterSize = worldMeterSize;
				terrainRendererPassData.heightScale = worldHeightScale;
				terrainRendererPassData.culledPatchListIndex = culledPatchList->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.heightMapIndex = heightMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.albedoMapIndex = 0u;
				terrainRendererPassData.normalMapIndex = normalMap->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::TerrainRendererPassData));
				memcpy(passDataAlloc.cpuAddress, &terrainRendererPassData, sizeof(TerrainPass::TerrainRendererPassData));

				IndirectDrawIndexed indirectDrawIndexed{};
				indirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexed.passDataAddress = passDataAlloc.gpuAddress;
				indirectDrawIndexed.vertexBufferView = patchMesh->GetVertexBuffer()->GetVBDescriptor();
				indirectDrawIndexed.indexBufferView = patchMesh->GetIndexBuffer()->GetIBDescriptor();
				indirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = patchMesh->GetIndexCount();
				indirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch += commandBuffer.TransitionImmediately(heightMap.Get(), GHL::EResourceState::PixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);
				
				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDrawIndexed, sizeof(IndirectDrawIndexed));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.CopyBufferRegion(indirectArgs, 
					sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u + sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(UINT), 
					culledPatchList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				auto& gBufferAlbedoMetalnessDesc = gBufferAlbedoMetalness->GetResourceFormat().GetTextureDesc();
				uint16_t width  = static_cast<uint16_t>(gBufferAlbedoMetalnessDesc.width);
				uint16_t height = static_cast<uint16_t>(gBufferAlbedoMetalnessDesc.height);
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
				commandBuffer.SetGraphicsPipelineState("TerrainRenderer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("TerrainRenderer", indirectArgs, 1u);	
			}
		);
	}

	void TerrainPass::UpdateNodeAndLodDescriptorArray() {
		uint32_t nodeCount{ 0u };

		lodDescriptors.resize(maxLOD + 1u);
		for (int32_t i = maxLOD; i >= 0; i--) {
			auto& lodDescriptor = lodDescriptors.at(i);

			uint32_t currDetailNodeSize = mostDetailNodeMeterSize * pow(2, i);
			uint32_t nodeCountPerAxis = terrainBuilderPassData.worldMeterSize.x / currDetailNodeSize;

			lodDescriptor.nodeSize = currDetailNodeSize;
			lodDescriptor.nodeStartOffset = nodeCount;
			lodDescriptor.nodeCount = nodeCountPerAxis * nodeCountPerAxis;

			nodeCount += lodDescriptor.nodeCount;
		}
		nodeDescriptors.resize(nodeCount);

		for (uint32_t i = 0; i < 5; i++) {
			for (uint32_t j = 0; j < 5; j++) {
				NodeLocation nodeLocation;
				nodeLocation.x = i;
				nodeLocation.y = j;
				maxLODNodeList.push_back(nodeLocation);
			}
		}

		isInitialized = false;
	}

	void TerrainPass::InitializePass(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		// Load PatchMesh From Memory
		{
			uint32_t size = 16u;
			float sizePerGrid = 0.5f;
			float totalMeterSize = size * sizePerGrid;
			float gridCount = size * size;
			float triangleCount = gridCount * 2u;

			float vOffset = -totalMeterSize * 0.5f;

			std::vector<Vertex> vertices;
			float uvStrip = 1.0f / size;
			for (uint32_t z = 0u; z <= size; z++) {
				for (uint32_t x = 0u; x <= size; x++) {
					vertices.emplace_back(
						Math::Vector3{ vOffset + x * 0.5f, 0u, vOffset + z * 0.5f },
						Math::Vector2{ x * uvStrip, z * uvStrip },
						Math::Vector3{}, Math::Vector3{}, Math::Vector3{}
					);
				}
			}

			std::vector<uint32_t> indices;
			indices.resize(triangleCount * 3u);
			for (uint32_t gridIndex = 0u; gridIndex < gridCount; gridIndex++) {
				uint32_t offset = gridIndex * 6u;
				uint32_t vIndex = (gridIndex / size) * (size + 1u) + (gridIndex % size);

				indices[offset] = vIndex;
				indices[offset + 1u] = vIndex + size + 1u;
				indices[offset + 2u] = vIndex + 1u;
				indices[offset + 3u] = vIndex + 1u;
				indices[offset + 4u] = vIndex + size + 1u;
				indices[offset + 5u] = vIndex + size + 2u;
			}

			Renderer::BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Renderer::Vertex);
			vbDesc.size = vbDesc.stride * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			Renderer::BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = ibDesc.stride * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			patchMesh = std::make_unique<Renderer::Mesh>(
				device,
				ResourceFormat{ device, vbDesc },
				ResourceFormat{ device, ibDesc },
				nullptr,
				nullptr);

			patchMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
		}

		// Load MinMaxHeightMap From Memory(目前读取的DDS文件，后续将DDS转换为XET)
		{
			minmaxHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/MinMaxHeightMap.dds");
			resourceStateTracker->StartTracking(minmaxHeightMap.Get());
		}

		// Load AlbedoMap From File
		/*
		{
			albedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/AlbedoMap.png"
			);
			resourceStateTracker->StartTracking(albedoMap.Get());
		}
		*/

		// Load NormalMap From File
		{
			normalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/NormalMap.png");
			resourceStateTracker->StartTracking(normalMap.Get());
		}
		

		// Load HeightMap From File
		{
			heightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/HeightMap.png"
			);
			resourceStateTracker->StartTracking(heightMap.Get());
		}
	}

}