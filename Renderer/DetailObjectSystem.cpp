#include "Renderer/DetailObjectSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	struct IndirectDispatch {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS lightDataAddress;
		D3D12_DISPATCH_ARGUMENTS  dispatchArguments;
		uint32_t pad1;
	};

	struct IndirectDrawIndexed {
	public:
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW      indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	DetailObjectSystem::DetailObjectSystem(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) {}

	void DetailObjectSystem::Initialize(RenderEngine* renderEngine) {
		// Tree 0
		mHierarchyInstancedStaticMeshs.emplace_back(std::make_unique<HierarchyInstancedStaticMesh>(
			mRenderEngine, "Tree0", "E:/MyProject/DXDance/Resources/Instances/Tree0", 512, 2048, 3u,
			{}));

		// Rock 0 ...


		mHierarchyInstancedStaticMeshSize = mHierarchyInstancedStaticMeshs.size();
		for (const auto& item : mHierarchyInstancedStaticMeshs) {
			mLodCount += item->GetLodGroupSize();
		}
		mCullClusterPassDatas.resize(mHierarchyInstancedStaticMeshSize);
		mCullStaticInstancePassDatas.resize(mHierarchyInstancedStaticMeshSize);
		mRenderStaticInstancePassDatas.resize(mLodCount);
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
					builder.WriteBuffer(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetDebugName());
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
				passDataAllocs.resize(mHierarchyInstancedStaticMeshSize);
				auto barrierBatch0 = GHL::ResourceBarrierBatch{};
				auto barrierBatch1 = GHL::ResourceBarrierBatch{};
				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					auto& currPassData = mCullClusterPassDatas.at(i);
					auto& currPassDataAlloc = passDataAllocs.at(i);

					currPassData.clusterNodeBufferIndex = currHierarchyInstancedStaticMesh->mGpuClusterNodesBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.visibleClusterNodeIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetUADescriptor()->GetHeapIndex();
					currPassData.clusterNodeBufferSize = currHierarchyInstancedStaticMesh->GetClusterNodeSize();
					currPassData.instanceVisibleDistance = currHierarchyInstancedStaticMesh->GetInstanceVisibleDistance();

					currPassDataAlloc = dynamicAllocator->Allocate(sizeof(DetailObjectSystem::CullClusterPassData), 256u);
					memcpy(currPassDataAlloc.cpuAddress, &currPassData, sizeof(DetailObjectSystem::CullClusterPassData));

					barrierBatch0 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					barrierBatch1 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				}
				commandBuffer.FlushResourceBarrier(barrierBatch0);

				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					commandBuffer.ClearCounterBuffer(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer, 0u);
				}
				commandBuffer.FlushResourceBarrier(barrierBatch1);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullClusterNode");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					auto& currPassDataAlloc = passDataAllocs.at(i);
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
					auto& visibleLodInstanceIndexBuffers = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers;
					builder.ReadBuffer(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetDebugName(), ShaderAccessFlag::NonPixelShader);
					for (auto& visibleLodInstanceIndexBuffer : visibleLodInstanceIndexBuffers) {
						builder.WriteBuffer(visibleLodInstanceIndexBuffer->GetDebugName());
					}
				}

				NewBufferProperties _CullStaticInstanceIndirectArgsProperties{};
				_CullStaticInstanceIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_CullStaticInstanceIndirectArgsProperties.size = sizeof(IndirectDispatch) * mHierarchyInstancedStaticMeshs.size();
				_CullStaticInstanceIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_CullStaticInstanceIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("CullStaticInstanceIndirectArgs", _CullStaticInstanceIndirectArgsProperties);
				builder.WriteCopyDstBuffer("CullStaticInstanceIndirectArgs");

				shaderManger.CreateComputeShader("CullStaticInstance",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/DetailObjectSystem/CullStaticInstance.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("CullStaticInstance",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// PassDataCB
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });	// LightDataSB
						proxy.AddIndirectArgument(GHL::IndirectDispatchArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDispatch));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* cullStaticInstanceIndirectArgs = resourceStorage->GetResourceByName("CullStaticInstanceIndirectArgs")->GetBuffer();

				std::vector<LinearAllocation> passDataAllocs;
				std::vector<IndirectDispatch> mutipleIndirectDispatches;
				passDataAllocs.resize(mHierarchyInstancedStaticMeshSize);
				mutipleIndirectDispatches.resize(mHierarchyInstancedStaticMeshSize);

				// 将visibleLod0InstanceIndexBuffer的CounterBuffer转换为CopyDestination
				auto barrierBatch0 = GHL::ResourceBarrierBatch{};
				// 将visibleLod0InstanceIndexBuffer的CounterBuffer转换为UnorderedAccess
				auto barrierBatch1 = GHL::ResourceBarrierBatch{};
				// 将GpuCulledVisibleClusterNodesIndexBuffer的CounterBuffer转换为CopySource
				auto barrierBatch2 = GHL::ResourceBarrierBatch{};

				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					// Allocate PassData
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					auto& currPassData = mCullStaticInstancePassDatas.at(i);
					auto& currPassDataAlloc = passDataAllocs.at(i);
					const auto& lodDistances = currHierarchyInstancedStaticMesh->GetLodDistances();

					currPassData.clusterNodeBufferIndex = currHierarchyInstancedStaticMesh->mGpuClusterNodesBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.transformedBoundingBoxBufferIndex = currHierarchyInstancedStaticMesh->mGpuTransformedBoundingBoxBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.sortedInstancesBufferIndex = currHierarchyInstancedStaticMesh->mGpuSortedInstancesBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.visibleClusterNodeIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetSRDescriptor()->GetHeapIndex();
					currPassData.visibleLod0InstanceIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(0)->GetUADescriptor()->GetHeapIndex();
					currPassData.visibleLod1InstanceIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(1)->GetUADescriptor()->GetHeapIndex();
					currPassData.visibleLod2InstanceIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(2)->GetUADescriptor()->GetHeapIndex();
					currPassData.instanceCountPerCluster = currHierarchyInstancedStaticMesh->GetInstanceCountPerCluster();
					currPassData.lod0InstanceVisibleDistance = lodDistances.at(0);
					currPassData.lod1InstanceVisibleDistance = lodDistances.at(1);
					currPassData.lod2InstanceVisibleDistance = lodDistances.at(2);
					currPassData.instanceVisibleDistance = currHierarchyInstancedStaticMesh->GetInstanceVisibleDistance();
					currPassData.totalInstanceCount = currHierarchyInstancedStaticMesh->GetTotalInstanceCount();

					currPassDataAlloc = dynamicAllocator->Allocate(sizeof(DetailObjectSystem::CullStaticInstancePassData), 256u);
					memcpy(currPassDataAlloc.cpuAddress, &currPassData, sizeof(DetailObjectSystem::CullStaticInstancePassData));

					for (int32_t lodIndex = 0; lodIndex < currHierarchyInstancedStaticMesh->GetLodGroupSize(); lodIndex++) {
						barrierBatch0 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex)->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
						barrierBatch1 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex)->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					}
					barrierBatch2 += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer->GetCounterBuffer(), GHL::EResourceState::CopySource);
				
					// Fill IndirectDispatch
					auto& indirectDispatch = mutipleIndirectDispatches.at(i);
					int32_t instanceCountPerCluster = currHierarchyInstancedStaticMesh->GetInstanceCountPerCluster();
					uint32_t threadGroupCountY = (instanceCountPerCluster + smCullStaticInstanceThreadSizeInGroup - 1) / smCullStaticInstanceThreadSizeInGroup;
					indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
					indirectDispatch.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
					indirectDispatch.passDataAddress = passDataAllocs.at(i).gpuAddress;
					indirectDispatch.dispatchArguments.ThreadGroupCountX = 0u;
					indirectDispatch.dispatchArguments.ThreadGroupCountY = threadGroupCountY;
					indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;
				}

				commandBuffer.FlushResourceBarrier(barrierBatch0);
				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					for (int32_t lodIndex = 0; lodIndex < currHierarchyInstancedStaticMesh->GetLodGroupSize(); lodIndex++) {
						commandBuffer.ClearCounterBuffer(currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex), 0u);
					}
				}
				commandBuffer.FlushResourceBarrier(barrierBatch1);
				commandBuffer.FlushResourceBarrier(barrierBatch2);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(cullStaticInstanceIndirectArgs, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(cullStaticInstanceIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.UploadBufferRegion(cullStaticInstanceIndirectArgs, 0u, mutipleIndirectDispatches.data(), sizeof(IndirectDispatch)* mutipleIndirectDispatches.size());
				for (int32_t i = 0; i < mHierarchyInstancedStaticMeshSize; i++) {
					auto* currHierarchyInstancedStaticMesh = mHierarchyInstancedStaticMeshs.at(i).get();
					auto& visibleClusterNodeIndexBuffer = currHierarchyInstancedStaticMesh->mGpuCulledVisibleClusterNodesIndexBuffer;
					uint64_t globalByteOffset = sizeof(IndirectDispatch) * i;
					uint64_t localByteOffset = sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u;
					commandBuffer.CopyBufferRegion(cullStaticInstanceIndirectArgs, globalByteOffset + localByteOffset,
						visibleClusterNodeIndexBuffer->GetCounterBuffer(), 0, sizeof(uint32_t));
				}
				commandBuffer.ClearCounterBuffer(cullStaticInstanceIndirectArgs, mHierarchyInstancedStaticMeshSize);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(cullStaticInstanceIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(cullStaticInstanceIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullStaticInstance");
				commandBuffer.ExecuteIndirect("CullStaticInstance", cullStaticInstanceIndirectArgs, mHierarchyInstancedStaticMeshSize);
			});

		/*
		* 渲染可见的Instance
		*/
		renderGraph->AddPass(
			"RenderStaticInstance",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("NormalRoughness");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteDepthStencil("DepthStencil");

				for (auto& currHierarchyInstancedStaticMesh : mHierarchyInstancedStaticMeshs) {
					auto& visibleLodInstanceIndexBuffers = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers;
					for (auto& visibleLodInstanceIndexBuffer : visibleLodInstanceIndexBuffers) {
						builder.ReadBuffer(visibleLodInstanceIndexBuffer->GetDebugName(), ShaderAccessFlag::NonPixelShader);
					}
				}

				NewBufferProperties _StaticInstanceRendererIndirectArgsProperties{};
				_StaticInstanceRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_StaticInstanceRendererIndirectArgsProperties.size = _StaticInstanceRendererIndirectArgsProperties.stride * mLodCount;
				_StaticInstanceRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_StaticInstanceRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("StaticInstanceRendererIndirectArgs", _StaticInstanceRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("StaticInstanceRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("RenderStaticInstance",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/DetailObjectSystem/RenderStaticInstance.hlsl";
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

				commandSignatureManger.CreateCommandSignature("RenderStaticInstance",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });	// FrameDataCB
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });	// PassDataCB
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });	// LightDataSB
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});			// VertexBufferView
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});			// IndexBufferView
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});				// DrawIndexedArgument
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
				auto* staticInstanceRendererIndirectArgs = resourceStorage->GetResourceByName("StaticInstanceRendererIndirectArgs")->GetBuffer();

				std::vector<LinearAllocation> passDataAllocs;
				std::vector<IndirectDrawIndexed> mutipleIndirectDrawIndexeds;
				passDataAllocs.resize(mLodCount);
				mutipleIndirectDrawIndexeds.resize(mLodCount);
				
				int32_t currPassDataIndex = 0;
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(staticInstanceRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				for (auto& currHierarchyInstancedStaticMesh : mHierarchyInstancedStaticMeshs) {
					for (int32_t lodIndex = 0; lodIndex < currHierarchyInstancedStaticMesh->GetLodGroupSize(); lodIndex++) {
						// Allocate PassData
						auto& currPassData = mRenderStaticInstancePassDatas.at(currPassDataIndex);
						auto& currPassDataAlloc = passDataAllocs.at(currPassDataIndex);

						currPassData.instanceAlbedoMapIndex = currHierarchyInstancedStaticMesh->mAlbedoMap->GetSRDescriptor()->GetHeapIndex();
						currPassData.instanceNormalMapIndex = currHierarchyInstancedStaticMesh->mNormalMap->GetSRDescriptor()->GetHeapIndex();
						currPassData.instanceRoughnessMapIndex = currHierarchyInstancedStaticMesh->mRoughnessMap->GetSRDescriptor()->GetHeapIndex();
						currPassData.instanceAoMapIndex = currHierarchyInstancedStaticMesh->mAoMap->GetSRDescriptor()->GetHeapIndex();
						currPassData.transformsBufferIndex = currHierarchyInstancedStaticMesh->mGpuTransformsBuffer->GetSRDescriptor()->GetHeapIndex();
						currPassData.visibleInstanceIndexBufferIndex = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex)->GetSRDescriptor()->GetHeapIndex();

						currPassDataAlloc = dynamicAllocator->Allocate(sizeof(DetailObjectSystem::RenderStaticInstancePassData), 256u);
						memcpy(currPassDataAlloc.cpuAddress, &currPassData, sizeof(DetailObjectSystem::RenderStaticInstancePassData));

						// Fill IndirectDrawIndexed
						auto* lodMesh = currHierarchyInstancedStaticMesh->GetLodGroups().at(lodIndex)->GetFirstMesh();
						auto& currIndirectDrawIndexed = mutipleIndirectDrawIndexeds.at(currPassDataIndex);
						currIndirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
						currIndirectDrawIndexed.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
						currIndirectDrawIndexed.passDataAddress = currPassDataAlloc.gpuAddress;
						currIndirectDrawIndexed.vertexBufferView = lodMesh->GetVertexBuffer()->GetVBDescriptor();
						currIndirectDrawIndexed.indexBufferView = lodMesh->GetIndexBuffer()->GetIBDescriptor();
						currIndirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = lodMesh->GetIndexCount();
						currIndirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
						currIndirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
						currIndirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
						currIndirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

						// Fill BarrierBatch
						barrierBatch += commandBuffer.TransitionImmediately(currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex)->GetCounterBuffer(), GHL::EResourceState::CopySource);

						currPassDataIndex++;
					}
				}

				commandBuffer.FlushResourceBarrier(barrierBatch);
				commandBuffer.UploadBufferRegion(staticInstanceRendererIndirectArgs, 0u, mutipleIndirectDrawIndexeds.data(), mutipleIndirectDrawIndexeds.size() * sizeof(IndirectDrawIndexed));
				// Copy Instance Count
				currPassDataIndex = 0;
				for (auto& currHierarchyInstancedStaticMesh : mHierarchyInstancedStaticMeshs) {
					for (int32_t lodIndex = 0; lodIndex < currHierarchyInstancedStaticMesh->GetLodGroupSize(); lodIndex++) {
						uint64_t globalByteOffset = currPassDataIndex * sizeof(IndirectDrawIndexed);
						uint64_t localByteOffset = 3u * sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(UINT);
						auto& visibleLodInstanceIndexBuffer = currHierarchyInstancedStaticMesh->mGpuCulledVisibleInstanceIndexBuffers.at(lodIndex);

						commandBuffer.CopyBufferRegion(staticInstanceRendererIndirectArgs, globalByteOffset + localByteOffset,
							visibleLodInstanceIndexBuffer->GetCounterBuffer(), 0, sizeof(uint32_t));
						currPassDataIndex++;
					}
				}
				commandBuffer.ClearCounterBuffer(staticInstanceRendererIndirectArgs, mLodCount);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(staticInstanceRendererIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(staticInstanceRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

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
				commandBuffer.SetGraphicsPipelineState("RenderStaticInstance");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("RenderStaticInstance", staticInstanceRendererIndirectArgs, mLodCount);
			});
	}

}