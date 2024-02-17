#include "Renderer/TerrainPipelinePass.h"
#include "Renderer/TerrainBackend.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/Misc.h"

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
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW      indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	TerrainPipelinePass::TerrainPipelinePass(TerrainRenderer* renderer)
	: mRenderer(renderer) 
	, mTerrainSetting(renderer->mTerrainSetting) {

		CreateGraphicsObject();
	}
	
	TerrainPipelinePass::~TerrainPipelinePass() {
	}

	void TerrainPipelinePass::AddPass() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* computeQueue = renderEngine->mComputeQueue.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* frameTracker = renderEngine->mFrameTracker.get();
		auto* resourceStorage = renderEngine->mPipelineResourceStorage;

		auto& finalOutputDesc = resourceStorage->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		uint32_t _MaxNodeListSize = 600u;
		uint32_t _TmpNodeListSize = 150u;

		renderGraph->AddPass(
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
				_ConsumeNodeListProperties.stride = sizeof(uint32_t) * 2u;	// uint2 x,y
				_ConsumeNodeListProperties.size = _TmpNodeListSize * _ConsumeNodeListProperties.stride;
				_ConsumeNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_ConsumeNodeListProperties.aliased = false;
				builder.DeclareBuffer("ConsumeNodeList", _ConsumeNodeListProperties);
				builder.WriteBuffer("ConsumeNodeList");

				NewBufferProperties _AppendNodeListProperties;
				_AppendNodeListProperties.stride = sizeof(uint32_t) * 2u;	// uint2 x,y
				_AppendNodeListProperties.size = _TmpNodeListSize * _AppendNodeListProperties.stride;
				_AppendNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_AppendNodeListProperties.aliased = false;
				builder.DeclareBuffer("AppendNodeList", _AppendNodeListProperties);
				builder.WriteBuffer("AppendNodeList");

				NewBufferProperties _FinalNodeListProperties;
				_FinalNodeListProperties.stride = sizeof(uint32_t) * 3u;	// uint3 x,y,lod
				_FinalNodeListProperties.size = _MaxNodeListSize * _FinalNodeListProperties.stride;
				_FinalNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_FinalNodeListProperties.aliased = false;
				builder.DeclareBuffer("FinalNodeList", _FinalNodeListProperties);
				builder.WriteBuffer("FinalNodeList");

				shaderManger.CreateComputeShader("TraverseQuadTree",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainQuadTreeBuilder.hlsl";
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
				// 检测TerrainBackend是否存在资源更新任务
				{
					auto* terrainBackend = mRenderer->mTerrainBackend.get();
					auto& recordedGpuCommands = terrainBackend->GetRecordedGpuCommands();
					
					TerrainBackend::RecordedGpuCommand recordedGpuCommand{};
					if (recordedGpuCommands.TryPop(recordedGpuCommand)) {
						// 向GPU输送已被录制好的命令
						recordedGpuCommand.copyQueue->ExecuteCommandList(recordedGpuCommand.copyCommandList->D3DCommandList());
						recordedGpuCommand.copyQueue->SignalFence(*recordedGpuCommand.copyFence, recordedGpuCommand.copyFenceExpectedValue);
						recordedGpuCommand.computeQueue->ExecuteCommandList(recordedGpuCommand.computeCommandList->D3DCommandList());
						recordedGpuCommand.computeQueue->SignalFence(*recordedGpuCommand.computeFence, recordedGpuCommand.computeFenceExpectedValue);

						// 让ComputeQueue等待命令的完成
						computeQueue->WaitFence(*recordedGpuCommand.copyFence, recordedGpuCommand.copyFenceExpectedValue);
						computeQueue->WaitFence(*recordedGpuCommand.computeFence, recordedGpuCommand.computeFenceExpectedValue);
					}
				}

				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage  = renderContext.resourceStorage;
				auto* commandSigManger = renderContext.commandSignatureManger;

				auto* consumeNodeList = resourceStorage->GetResourceByName("ConsumeNodeList")->GetBuffer();
				auto* appendNodeList  = resourceStorage->GetResourceByName("AppendNodeList")->GetBuffer();
				auto* finalNodeList   = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* indirectArgs    = resourceStorage->GetResourceByName("TraverseQuadTreeIndirectArgs")->GetBuffer();

				mTerrainBuilderPassData.nodeEvaluationC.x  = mTerrainSetting.smNodeEvaluationC;
				mTerrainBuilderPassData.terrainMeterSize   = mTerrainSetting.smTerrainMeterSize;
				mTerrainBuilderPassData.terrainHeightScale = mTerrainSetting.smTerrainHeightScale;
				mTerrainBuilderPassData.useFrustumCull     = mTerrainSetting.smUseFrustumCull;
				mTerrainBuilderPassData.currPassLOD        = mTerrainSetting.smMaxLOD;
				mTerrainBuilderPassData.consumeNodeListIndex = consumeNodeList->GetUADescriptor()->GetHeapIndex();
				mTerrainBuilderPassData.appendNodeListIndex  = appendNodeList->GetUADescriptor()->GetHeapIndex();
				mTerrainBuilderPassData.finalNodeListIndex   = finalNodeList->GetUADescriptor()->GetHeapIndex();
				mTerrainBuilderPassData.nodeDescriptorListIndex = mRenderer->mTerrainNodeDescriptorBuffer->GetSRDescriptor()->GetHeapIndex();
				mTerrainBuilderPassData.lodDescriptorListIndex  = mRenderer->mTerrainLodDescriptorBuffer->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPipelinePass::TerrainBuilderPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mTerrainBuilderPassData, sizeof(TerrainPipelinePass::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress  = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = mMaxLodNodeList.size();
				indirectDispatch.dispatchArguments.ThreadGroupCountY = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(consumeNodeList, 0u, mMaxLodNodeList.data(), mMaxLodNodeList.size() * sizeof(TerrainPipelinePass::NodeLocation));
				commandBuffer.ClearCounterBuffer(consumeNodeList, mMaxLodNodeList.size());
				commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
				commandBuffer.ClearCounterBuffer(finalNodeList, 0u);
				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TraverseQuadTree");
				for (int32_t lod = mTerrainSetting.smMaxLOD; lod >= 0; lod--) {
					if (lod != mTerrainSetting.smMaxLOD) {
						// 更新TerrainBuilderPassData
						uint32_t tempValue = mTerrainBuilderPassData.consumeNodeListIndex;
						mTerrainBuilderPassData.consumeNodeListIndex = mTerrainBuilderPassData.appendNodeListIndex;
						mTerrainBuilderPassData.appendNodeListIndex = tempValue;
						mTerrainBuilderPassData.currPassLOD = lod;

						passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPipelinePass::TerrainBuilderPassData), 256u);
						memcpy(passDataAlloc.cpuAddress, &mTerrainBuilderPassData, sizeof(TerrainPipelinePass::TerrainBuilderPassData));

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
			});

		renderGraph->AddPass(
			"BuildPatches",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("FinalNodeList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _CulledPatchListProperties{};
				_CulledPatchListProperties.stride = sizeof(TerrainPipelinePass::RenderPatch);
				_CulledPatchListProperties.size = _MaxNodeListSize * _CulledPatchListProperties.stride * 64;
				_CulledPatchListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_CulledPatchListProperties.aliased = false;
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
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainQuadTreeBuilder.hlsl";
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
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage  = renderContext.resourceStorage;

				auto* finalNodeList   = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* culledPatchList = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs    = resourceStorage->GetResourceByName("BuildPatchesIndirectArgs")->GetBuffer();

				mTerrainBuilderPassData.finalNodeListIndex   = finalNodeList->GetSRDescriptor()->GetHeapIndex();
				mTerrainBuilderPassData.culledPatchListIndex = culledPatchList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPipelinePass::TerrainBuilderPassData));
				memcpy(passDataAlloc.cpuAddress, &mTerrainBuilderPassData, sizeof(TerrainPipelinePass::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress  = passDataAlloc.gpuAddress;
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
			});

		renderGraph->AddPass(
			"TerrainFeedbackRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("CulledPatchList", ShaderAccessFlag::PixelShader);

				builder.WriteRenderTarget("TerrainFeedback");

				shaderManger.CreateGraphicsShader("TerrainFeedbackRenderer",
					[&](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = shaderPath + "TerrainRenderer/TerrainFeedbackRenderer.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = { DXGI_FORMAT_R16G16B16A16_UINT };
					});

				commandSignatureManger.CreateCommandSignature("TerrainFeedbackRenderer",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {

			});

		renderGraph->AddPass(
			"TerrainRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("CulledPatchList", ShaderAccessFlag::PixelShader);

				builder.WriteRenderTarget("GBufferAlbedoMetalness");
				builder.WriteRenderTarget("GBufferPositionEmission");
				builder.WriteRenderTarget("GBufferNormalRoughness");
				builder.WriteRenderTarget("GBufferMotionVector");
				builder.WriteRenderTarget("GBufferViewDepth");
				builder.WriteDepthStencil("GBufferDepthStencil");

				NewBufferProperties _TerrainRendererIndirectArgsProperties{};
				_TerrainRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.size = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_TerrainRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("TerrainRendererIndirectArgs", _TerrainRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("TerrainRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("TerrainRenderer",
					[&](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = shaderPath + "TerrainRenderer/TerrainQuadTreeRenderer.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						// proxy.rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
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
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage  = renderContext.resourceStorage;

				auto* culledPatchList = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs    = resourceStorage->GetResourceByName("TerrainRendererIndirectArgs")->GetBuffer();
				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector     = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil     = resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();
				// auto* pageTableTexture      = resourceStorage->GetResourceByName("PageTableTexture")->GetTexture();
				// auto* physicalTextureAlbedo = resourceStorage->GetResourceByName("PhysicalTextureAlbedo")->GetTexture();
				// auto* physicalTextureNormal = resourceStorage->GetResourceByName("PhysicalTextureNormal")->GetTexture();

				/*
				// 更新Rvt参数
				uint32_t tileCountPerAxis = mRvtSystem->GetTileCountPerAxis();
				uint32_t maxMipLevel = mRvtSystem->GetMaxMipLevel();
				uint32_t tileSize = mRvtSystem->GetTileSize();
				uint32_t paddingSize = mRvtSystem->GetPaddingSize();
				uint32_t physicalTextureSize = mRvtSystem->GetPhysicalTextureSize();

				mTerrainRendererPassData.vtFeedbackParams = Math::Vector4{ (float)tileCountPerAxis, (float)(tileCountPerAxis * tileSize), (float)maxMipLevel, 0.0f };
				mTerrainRendererPassData.vtRealRect = mRvtSystem->GetCurrRvtRect();
				mTerrainRendererPassData.vtPhysicalMapParams = Math::Vector4{ (float)paddingSize, (float)tileSize, (float)physicalTextureSize, (float)physicalTextureSize };
				mTerrainRendererPassData.pageTableMapIndex = pageTableTexture->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.physicalAlbedoMapIndex = physicalTextureAlbedo->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.physicalNormalMapIndex = physicalTextureNormal->GetSRDescriptor()->GetHeapIndex();
				*/

				mTerrainRendererPassData.lodDebug             = mTerrainSetting.smUseLodDebug;
				mTerrainRendererPassData.terrainMeterSize     = mTerrainSetting.smTerrainMeterSize;
				mTerrainRendererPassData.terrainHeightScale   = mTerrainSetting.smTerrainHeightScale;
				mTerrainRendererPassData.culledPatchListIndex = culledPatchList->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.nodeDescriptorListIndex = mRenderer->mTerrainNodeDescriptorBuffer->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.lodDescriptorListIndex  = mRenderer->mTerrainLodDescriptorBuffer->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.terrainAlbedoTextureArrayIndex;
				mTerrainRendererPassData.terrainNormalTextureArrayIndex;
				mTerrainRendererPassData.terrainHeightMapAtlasIndex     = mRenderer->GetFarTerrainHeightMapAtlas()->GetTextureAtlas()->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.terrainAlbedoMapAtlasIndex     = mRenderer->GetFarTerrainAlbedoMapAtlas()->GetTextureAtlas()->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.terrainNormalMapAtlasIndex     = mRenderer->GetFarTerrainNormalMapAtlas()->GetTextureAtlas()->GetSRDescriptor()->GetHeapIndex();
				mTerrainRendererPassData.terrainAtlasTileCountPerAxis   = mRenderer->GetFarTerrainHeightMapAtlas()->GetTileCountPerAxis();
				mTerrainRendererPassData.terrainAtlasTileWidthInPixels  = mRenderer->GetFarTerrainHeightMapAtlas()->GetTileSize();
				mTerrainRendererPassData.terrainPatchVertexCountPerAxis = 9u;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainPipelinePass::TerrainRendererPassData));
				memcpy(passDataAlloc.cpuAddress, &mTerrainRendererPassData, sizeof(TerrainPipelinePass::TerrainRendererPassData));

				IndirectDrawIndexed indirectDrawIndexed{};
				indirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexed.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				indirectDrawIndexed.passDataAddress  = passDataAlloc.gpuAddress;
				indirectDrawIndexed.vertexBufferView = mPatchMeshVertexBuffer->GetVBDescriptor();
				indirectDrawIndexed.indexBufferView  = mPatchMeshIndexBuffer->GetIBDescriptor();
				indirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = mPatchMeshIndexCount;
				indirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				// barrierBatch += commandBuffer.TransitionImmediately(terrainHeightMap, GHL::EResourceState::PixelShaderAccess);
				// barrierBatch += commandBuffer.TransitionImmediately(pageTableTexture, GHL::EResourceState::PixelShaderAccess);
				// barrierBatch += commandBuffer.TransitionImmediately(physicalTextureAlbedo, GHL::EResourceState::PixelShaderAccess);
				// barrierBatch += commandBuffer.TransitionImmediately(physicalTextureNormal, GHL::EResourceState::PixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDrawIndexed, sizeof(IndirectDrawIndexed));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(UINT), culledPatchList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				barrierBatch =  commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				uint16_t width  = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);

				commandBuffer.SetRenderTargets(
					{
						gBufferAlbedoMetalness,
						gBufferPositionEmission,
						gBufferNormalRoughness,
						gBufferMotionVector,
						gBufferViewDepth,
					},
					gBufferDepthStencil);

				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("TerrainRenderer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("TerrainRenderer", indirectArgs, 1u);
			});
	}

	void TerrainPipelinePass::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;

		auto* device = renderEngine->mDevice.get();
		auto* dstorageQueue = renderEngine->mDStorageMemQueue.get();
		auto* dstorageFence = renderEngine->mDStorageFence.get();
		auto* frameTracker = renderEngine->mFrameTracker.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		// Create Node List In Max Lod
		{
			uint32_t nodeCountPerAxisInMaxLod = mTerrainSetting.smTerrainMeterSize / mTerrainSetting.smMaxLODNodeMeterSize;
			for (uint32_t i = 0; i < nodeCountPerAxisInMaxLod; i++) {
				for (uint32_t j = 0; j < nodeCountPerAxisInMaxLod; j++) {
					NodeLocation nodeLocation;
					nodeLocation.x = i;
					nodeLocation.y = j;
					mMaxLodNodeList.push_back(nodeLocation);
				}
			}
		}

		// Load PatchMesh From Memory By DStorageQueue
		{
			// uint32_t size = 16u;
			// float sizePerGrid = 0.5f;
			uint32_t size = 8u;
			float sizePerGrid = 1u;
			float totalMeterSize = size * sizePerGrid;
			float gridCount = size * size;
			float triangleCount = gridCount * 2u;

			float uOffset = -totalMeterSize * 0.5f;
			float vOffset = totalMeterSize * 0.5f;

			std::vector<Vertex> vertices;
			float uvStrip = 1.0f / size;
			for (uint32_t z = 0u; z <= size; z++) {
				for (uint32_t x = 0u; x <= size; x++) {
					vertices.emplace_back(
						Math::Vector3{ uOffset + x * sizePerGrid, 0u, uOffset + z * sizePerGrid },
						Math::Vector2{ x * uvStrip, z * uvStrip },
						Math::Vector3{}, 
						Math::Vector3{}, 
						Math::Vector3{}, 
						Math::Vector4{ (float)x, (float)(size - z), 0.0f, 0.0f }		// color
					);
					vertices.back().uv = Math::Vector2{ x * uvStrip, z * uvStrip };
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

			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(Renderer::Vertex);
			vertexBufferDesc.size = vertexBufferDesc.stride * vertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			mPatchMeshVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * indices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			mPatchMeshIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);
			mPatchMeshIndexCount  = indices.size();

			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(vertices.data()), vertexBufferDesc.size, mPatchMeshVertexBuffer.Get(), 0u);
			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(indices.data()), indexBufferDesc.size, mPatchMeshIndexBuffer.Get(), 0u);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence);
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}
	}

}