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

		uint32_t _NodeListSize = 100u;

		UpdateNodeAndLodDescriptorArray();

		renderGraph.AddPass(
			"TraverseQuadTree",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				// Debug代码，后续要删除
				builder.WriteRenderTarget("FinalOutput");

				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				shaderManger.CreateComputeShader("TraverseQuadTree",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TraverseQuadTree.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("CmdSig_TraverseQuadTree",
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

				NewBufferProperties _CurrLODNodeListProperties;
				_CurrLODNodeListProperties.stride = sizeof(uint32_t) * 2u; // uint2
				_CurrLODNodeListProperties.size = _NodeListSize * _CurrLODNodeListProperties.stride;
				_CurrLODNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("CurrLODNodeList", _CurrLODNodeListProperties);
				builder.WriteBuffer("CurrLODNodeList");

				NewBufferProperties _NextLODNodeListProperties;
				_NextLODNodeListProperties.stride = sizeof(uint32_t) * 2u; // uint2
				_NextLODNodeListProperties.size = _NodeListSize * _NextLODNodeListProperties.stride;
				_NextLODNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("NextLODNodeList", _NextLODNodeListProperties);
				builder.WriteBuffer("NextLODNodeList");

				NewBufferProperties _FinalNodeListProperties;
				_FinalNodeListProperties.stride = sizeof(uint32_t) * 3u; // uint3
				_FinalNodeListProperties.size = _NodeListSize * _FinalNodeListProperties.stride;
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
				_LODDescriptorListProperties.stride = sizeof(LodDescriptor);
				_LODDescriptorListProperties.size = lodDescriptors.size() * _LODDescriptorListProperties.stride;
				_LODDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("LODDescriptorList", _LODDescriptorListProperties);
				builder.WriteBuffer("LODDescriptorList");
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator       = renderContext.dynamicAllocator;
				auto* resourceStorage        = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;
				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("CmdSig_TraverseQuadTree");

				auto* currLODNodeList    = resourceStorage->GetResourceByName("CurrLODNodeList")->GetBuffer();
				auto* nextLODNodeList    = resourceStorage->GetResourceByName("NextLODNodeList")->GetBuffer();
				auto* finalNodeList      = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* nodeDescriptorList = resourceStorage->GetResourceByName("NodeDescriptorList")->GetBuffer();
				auto* lodDescriptorList  = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();
				auto* indirectArgs       = resourceStorage->GetResourceByName("TraverseQuadTreeIndirectArgs")->GetBuffer();

				passData.currPassLOD = maxLOD;
				passData.currLODNodeListIndex    = currLODNodeList->GetUADescriptor()->GetHeapIndex();
				passData.nextLODNodeListIndex    = nextLODNodeList->GetUADescriptor()->GetHeapIndex();
				passData.finalNodeListIndex      = finalNodeList->GetUADescriptor()->GetHeapIndex();
				passData.nodeDescriptorListIndex = nodeDescriptorList->GetUADescriptor()->GetHeapIndex();
				passData.lodDescriptorListIndex  = lodDescriptorList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::PassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &passData, sizeof(TerrainPass::PassData));

				IndirectCommand indirectCommand{};
				indirectCommand.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectCommand.passDataAddress = passDataAlloc.gpuAddress;
				indirectCommand.dispatchArguments.ThreadGroupCountX = maxLODNodeList.size();
				indirectCommand.dispatchArguments.ThreadGroupCountY = 1u;
				indirectCommand.dispatchArguments.ThreadGroupCountZ = 1u;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TraverseQuadTree");

				if (!isInitialized) {
					auto barrierBatch = commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.UploadBufferRegion(nodeDescriptorList, 0u, 
						nodeDescriptors.data(), nodeDescriptors.size() * sizeof(TerrainPass::NodeDescriptor)
					);
					commandBuffer.UploadBufferRegion(lodDescriptorList, 0u,
						lodDescriptors.data(), lodDescriptors.size() * sizeof(TerrainPass::LodDescriptor)
					);

					isInitialized = true;
				}

				auto barrierBatch = commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(currLODNodeList, GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectCommand, sizeof(IndirectCommand));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);


				commandBuffer.UploadBufferRegion(currLODNodeList, 0u,
					maxLODNodeList.data(), maxLODNodeList.size() * sizeof(TerrainPass::NodeLocation)
				);
				commandBuffer.ClearCounterBuffer(currLODNodeList, maxLODNodeList.size());

				commandBuffer.ClearCounterBuffer(nextLODNodeList, 0u);
				commandBuffer.ClearCounterBuffer(finalNodeList, 0u);

				for (int32_t lod = maxLOD; lod >= maxLOD; lod--) {
					if (lod != maxLOD) {
					
					}
					// ExecuteIndirect
					auto indirectArgsBarrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
					indirectArgsBarrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					indirectArgsBarrierBatch += commandBuffer.TransitionImmediately(nextLODNodeList, GHL::EResourceState::UnorderedAccess);
					indirectArgsBarrierBatch += commandBuffer.TransitionImmediately(nextLODNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					indirectArgsBarrierBatch += commandBuffer.TransitionImmediately(finalNodeList, GHL::EResourceState::UnorderedAccess);
					indirectArgsBarrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(indirectArgsBarrierBatch);

					commandBuffer.D3DCommandList()->ExecuteIndirect(cmdSig, 1u, indirectArgs->D3DResource(), 0u, indirectArgs->GetCounterBuffer()->D3DResource(), 0u);
				}
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