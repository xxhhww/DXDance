#include "TerrainPass.h"
#include "RenderGraphBuilder.h"
#include "ShaderManger.h"
#include "CommandSignatureManger.h"
#include "LinearBufferAllocator.h"
#include "CommandBuffer.h"

namespace Renderer {

	struct IndirectCommand {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_DISPATCH_ARGUMENTS dispatchArguments;
		uint32_t pad;
	};

	void TerrainPass::AddPass(RenderGraph& renderGraph) {

		uint32_t _MaxNodeListSize = 200u;
		uint32_t _TmpNodeListSize = 50u;

		UpdateNodeAndLodDescriptorArray();

		renderGraph.AddPass(
			"TraverseQuadTree",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

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
						proxy.SetByteStride(sizeof(IndirectCommand));
					});

				NewBufferProperties _TraverseQuadTreeIndirectArgsProperties;
				_TraverseQuadTreeIndirectArgsProperties.stride = sizeof(IndirectCommand);
				_TraverseQuadTreeIndirectArgsProperties.size = sizeof(IndirectCommand);
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

				passData.currPassLOD = maxLOD;
				passData.consumeNodeListIndex    = consumeNodeList->GetUADescriptor()->GetHeapIndex();
				passData.appendNodeListIndex     = appendNodeList->GetUADescriptor()->GetHeapIndex();
				passData.finalNodeListIndex      = finalNodeList->GetUADescriptor()->GetHeapIndex();
				passData.nodeDescriptorListIndex = nodeDescriptorList->GetUADescriptor()->GetHeapIndex();
				passData.lodDescriptorListIndex  = lodDescriptorList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::PassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &passData, sizeof(TerrainPass::PassData));

				IndirectCommand indirectCommand{};
				indirectCommand.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectCommand.passDataAddress  = passDataAlloc.gpuAddress;
				indirectCommand.dispatchArguments.ThreadGroupCountX = maxLODNodeList.size();
				indirectCommand.dispatchArguments.ThreadGroupCountY = 1u;
				indirectCommand.dispatchArguments.ThreadGroupCountZ = 1u;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TraverseQuadTree");

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
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectCommand, sizeof(IndirectCommand));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.UploadBufferRegion(consumeNodeList, 0u,
					maxLODNodeList.data(), maxLODNodeList.size() * sizeof(TerrainPass::NodeLocation)
				);
				commandBuffer.ClearCounterBuffer(consumeNodeList, maxLODNodeList.size());

				commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
				commandBuffer.ClearCounterBuffer(finalNodeList, 0u);

				for (int32_t lod = maxLOD; lod >= 0; lod--) {
					if (lod != maxLOD) {
						// 更新PassData
						uint32_t tempValue = passData.consumeNodeListIndex;
						passData.consumeNodeListIndex = passData.appendNodeListIndex;
						passData.appendNodeListIndex  = tempValue;
						passData.currPassLOD = lod;

						passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::PassData), 256u);
						memcpy(passDataAlloc.cpuAddress, &passData, sizeof(TerrainPass::PassData));

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
				// Debug代码，后续要删除
				builder.WriteRenderTarget("FinalOutput");

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
						proxy.SetByteStride(sizeof(IndirectCommand));
					});

				builder.ReadBuffer("FinalNodeList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _CulledPatchListProperties{};
				_CulledPatchListProperties.stride = sizeof(TerrainPass::RenderPatch);
				_CulledPatchListProperties.size = _MaxNodeListSize * _CulledPatchListProperties.stride;
				_CulledPatchListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("CulledPatchList", _CulledPatchListProperties);
				builder.WriteBuffer("CulledPatchList");

				NewBufferProperties _BuildPatchesIndirectArgsProperties{};
				_BuildPatchesIndirectArgsProperties.stride = sizeof(IndirectCommand);
				_BuildPatchesIndirectArgsProperties.size = sizeof(IndirectCommand);
				_BuildPatchesIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_BuildPatchesIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("BuildPatchesIndirectArgs", _BuildPatchesIndirectArgsProperties);
				builder.WriteCopyDstBuffer("BuildPatchesIndirectArgs");
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator       = renderContext.dynamicAllocator;
				auto* resourceStorage        = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* finalNodeList   = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* culledPatchList = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs    = resourceStorage->GetResourceByName("BuildPatchesIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("BuildPatches");

				passData.finalNodeListIndex   = finalNodeList->GetSRDescriptor()->GetHeapIndex();
				passData.culledPatchListIndex = culledPatchList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::PassData));
				memcpy(passDataAlloc.cpuAddress, &passData, sizeof(TerrainPass::PassData));

				IndirectCommand indirectCommand{};
				indirectCommand.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectCommand.passDataAddress = passDataAlloc.gpuAddress;
				indirectCommand.dispatchArguments.ThreadGroupCountX = 1u;
				indirectCommand.dispatchArguments.ThreadGroupCountY = 1u;
				indirectCommand.dispatchArguments.ThreadGroupCountZ = 1u;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("BuildPatches");

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectCommand, sizeof(IndirectCommand));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, finalNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.ExecuteIndirect("BuildPatches", indirectArgs, 1u);
			}
		);
	}

	void TerrainPass::UpdateNodeAndLodDescriptorArray() {
		
		uint32_t nodeCount{ 0u };

		lodDescriptors.resize(maxLOD + 1u);
		for (int32_t i = maxLOD; i >= 0; i--) {
			auto& lodDescriptor = lodDescriptors.at(i);

			uint32_t currDetailNodeSize = mostDetailNodeSize * pow(2, i);
			uint32_t nodeCountPerAxis = passData.worldSize.x / currDetailNodeSize;

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
}