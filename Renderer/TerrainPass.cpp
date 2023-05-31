#include "TerrainPass.h"
#include "RenderGraphBuilder.h"
#include "ShaderManger.h"
#include "CommandSignatureManger.h"
#include "LinearBufferAllocator.h"

namespace Renderer {

	struct IndirectCommand {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_DISPATCH_ARGUMENTS dispatchArguments;
	};

	void TerrainPass::AddPass(RenderGraph& renderGraph) {

		uint32_t _NodeListSize = 50u;

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
				_LODDescriptorListProperties.stride = sizeof(LODDescriptor);
				_LODDescriptorListProperties.size = lodDescriptors.size() * _LODDescriptorListProperties.stride;
				_LODDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("LODDescriptorList", _LODDescriptorListProperties);
				builder.WriteBuffer("LODDescriptorList");
			},
			[=](CommandListWrap& commandList, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage  = renderContext.resourceStorage;
				auto* shaderManger     = renderContext.shaderManger;

				auto* shader = shaderManger->GetShader<ComputeShader>("TraverseQuadTree");
				auto* currLODNodeList    = resourceStorage->GetResourceByName("CurrLODNodeList")->GetBuffer();
				auto* nextLODNodeList    = resourceStorage->GetResourceByName("NextLODNodeList")->GetBuffer();
				auto* finalNodeList      = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* nodeDescriptorList = resourceStorage->GetResourceByName("NodeDescriptorList")->GetBuffer();
				auto* lodDescriptorList  = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();

				passData.currPassLOD = maxLOD;
				passData.currLODNodeListIndex    = currLODNodeList->GetUADescriptor()->GetHeapIndex();
				passData.nextLODNodeListIndex    = nextLODNodeList->GetUADescriptor()->GetHeapIndex();
				passData.finalNodeListIndex      = finalNodeList->GetUADescriptor()->GetHeapIndex();
				passData.nodeDescriptorListIndex = nodeDescriptorList->GetUADescriptor()->GetHeapIndex();
				passData.lodDescriptorListIndex  = lodDescriptorList->GetUADescriptor()->GetHeapIndex();

				auto dyAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::PassData), 256u);
				memcpy(dyAlloc.cpuAddress, &passData, sizeof(TerrainPass::PassData));

				IndirectCommand command{};
				command.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				command.passDataAddress = dyAlloc.gpuAddress;

				commandList->D3DCommandList()->SetComputeRootSignature(shaderManger->GetBaseD3DRootSignature());
				commandList->D3DCommandList()->SetPipelineState(shader->GetD3DPipelineState());
				commandList->D3DCommandList()->SetComputeRootConstantBufferView(0u, command.frameDataAddress);
				commandList->D3DCommandList()->SetComputeRootConstantBufferView(1u, command.passDataAddress);

				if (!isInitialized) {
					isInitialized = true;
					// Node 与 LOD 的 Descriptor由于用户的操作已经发生改变，需要对它们进行更新，记录下更新的命令
					dyAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::NodeDescriptor) * nodeDescriptors.size(), 256u);
					memcpy(dyAlloc.cpuAddress, nodeDescriptors.data(), dyAlloc.size);
					commandList->D3DCommandList()->CopyBufferRegion(nodeDescriptorList->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, dyAlloc.size);

					dyAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::LODDescriptor) * lodDescriptors.size(), 256u);
					memcpy(dyAlloc.cpuAddress, lodDescriptors.data(), dyAlloc.size);
					commandList->D3DCommandList()->CopyBufferRegion(lodDescriptorList->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, dyAlloc.size);
				}

				auto clearCounterBufferValue = [&](Renderer::Buffer* dstBuffer, uint32_t value) {
					dyAlloc = dynamicAllocator->Allocate(sizeof(uint32_t));
					memcpy(dyAlloc.cpuAddress, &value, dyAlloc.size);
					commandList->D3DCommandList()->CopyBufferRegion(dstBuffer->GetCounterBuffer()->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, sizeof(uint32_t));
				};
				clearCounterBufferValue(currLODNodeList, 5u * 5u);
				clearCounterBufferValue(nextLODNodeList, 0u);
				clearCounterBufferValue(finalNodeList, 0u);
				// clearCounterBufferValue(nodeDescriptorList, maxLOD * maxLOD);
				// clearCounterBufferValue(lodDescriptorList, maxLOD * maxLOD);

				dyAlloc = dynamicAllocator->Allocate(sizeof(TerrainPass::NodeLocation) * maxLODNodeList.size(), 256u);
				memcpy(dyAlloc.cpuAddress, maxLODNodeList.data(), dyAlloc.size);
				commandList->D3DCommandList()->CopyBufferRegion(currLODNodeList->D3DResource(), 0u, dyAlloc.backResource, dyAlloc.offset, dyAlloc.size);
				
				commandList->D3DCommandList()->Dispatch(25u, 1u, 1u);
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

			nodeCount += nodeCountPerAxis * nodeCountPerAxis;
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