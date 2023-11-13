#include "Renderer/DetailObjectSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	DetailObjectSystem::DetailObjectSystem(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) {}

	void DetailObjectSystem::Initialize(RenderEngine* renderEngine) {
		// Tree 0
		mHierarchyInstancedStaticMeshs.emplace_back(std::make_unique<HierarchyInstancedStaticMesh>(mRenderEngine,
			"Tree0", "E:/MyProject/DXDance/Resources/Instances/Tree0"));

		// Other Instances ...
		mCullClusterPassDatas.resize(mHierarchyInstancedStaticMeshs.size());
	}

	void DetailObjectSystem::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();

		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		/*
		* 剔除不可见ClusterNode
		*/
		renderGraph->AddPass(
			"CullClusterNode",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				for (auto& currHierarchyInstancedStaticMesh : mHierarchyInstancedStaticMeshs) {
					builder.WriteBuffer(currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetDebugName());
				}

				shaderManger.CreateComputeShader("CullClusterNode",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/DetailObjectSystem/CullClusterNode.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				std::vector<LinearAllocation> passDataAllocs;
				passDataAllocs.resize(mCullClusterPassDatas.size());
				auto barrierBatch0 = GHL::ResourceBarrierBatch{};
				auto barrierBatch1 = GHL::ResourceBarrierBatch{};
				for (int32_t index = 0; index < mCullClusterPassDatas.size(); index++) {
					CullClusterPassData& currPassData = mCullClusterPassDatas.at(index);
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(index).get();
					auto& currPassDataAlloc = passDataAllocs.at(index);

					currPassData.clusterNodeBufferIndex = currHierarchyInstancedStaticMesh->mGpuClusterNodesBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.visibleClusterNodeIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetUADescriptor()->GetHeapIndex();

					currPassDataAlloc = dynamicAllocator->Allocate(sizeof(DetailObjectSystem::CullClusterPassData), 256u);
					memcpy(currPassDataAlloc.cpuAddress, &currPassData, sizeof(DetailObjectSystem::CullClusterPassData));

					barrierBatch0 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					barrierBatch1 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				}
				commandBuffer.FlushResourceBarrier(barrierBatch0);

				for (int32_t index = 0; index < mCullClusterPassDatas.size(); index++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(index).get();
					commandBuffer.ClearCounterBuffer(currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetCounterBuffer(), 0u);
				}
				commandBuffer.FlushResourceBarrier(barrierBatch1);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullClusterNode");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				// commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				for (int32_t index = 0; index < mCullClusterPassDatas.size(); index++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(index).get();
					auto& currPassDataAlloc = passDataAllocs.at(index);
					commandBuffer.SetComputeRootCBV(1u, currPassDataAlloc.gpuAddress);

					uint32_t threadGroupCountX = (currHierarchyInstancedStaticMesh->GetClusterNodeSize() + smCullClusterNodeThreadSizeInGroup - 1) / smCullClusterNodeThreadSizeInGroup;
					commandBuffer.Dispatch(threadGroupCountX, 1u, 1u);
				}

			});

		/*
		* 剔除不可见Instance
		*/
		renderGraph->AddPass(
			"CullStaticInstance",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				for (auto& currHierarchyInstancedStaticMesh : mHierarchyInstancedStaticMeshs) {
					auto& visibleLodInstanceIndexBuffers = currHierarchyInstancedStaticMesh->mGpuCulledVisibleLodInstanceIndexBuffers;

					builder.ReadBuffer(currHierarchyInstancedStaticMesh->mGpuCulledClusterNodesIndexBuffer->GetDebugName(), ShaderAccessFlag::NonPixelShader);
					for (auto& visibleLodInstanceIndexBuffer : visibleLodInstanceIndexBuffers) {
						builder.WriteBuffer(visibleLodInstanceIndexBuffer->GetDebugName());
					}
				}
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				
			});

		/*
		* 渲染可见Instance
		*/
		renderGraph->AddPass(
			"RenderStaticInstance",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

			});
	}

}