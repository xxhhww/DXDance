#include "Renderer/VegetationSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/TerrainSystem.h"

namespace Renderer {

	struct IndirectDraw {
	public:
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_DRAW_ARGUMENTS		 drawArguments;
	};

	void VegetationSystem::Initialize(RenderEngine* renderEngine) {
		mRenderEngine = renderEngine;
		mTerrainSystem = renderEngine->mTerrainSystem.get();

		FillMiscData();

		TerrainSystem* terrainSystem = mRenderEngine->mTerrainSystem.get();

		mDataCacher = std::make_unique<VegetationDataCache>(mRenderEngine);
		mDataCacher->ConfigureGrassClusterCache(smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * 1.5f, smMaxGrassBladeCountPerAxis);
		mVegetationVirtualTable = std::make_unique<VegetationVirtualTable>(mDataCacher.get(), smVisibleGrassClusterCountPerAxis, smGrassClusterMeterSize, terrainSystem->worldMeterSize);
	}

	/*
	* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出，由RenderEngine的Update函数调用
	*/
	void VegetationSystem::Update(const Math::Vector2& cameraPosition) {
		mNeedBakedGpuGrassClusters.clear();
		mVisibleGpuGrassClusters.clear();

		std::vector<VegetationVirtualTable::VirtualTableCell&> changedVirtualTableCells;
		mVegetationVirtualTable->Update(cameraPosition, changedVirtualTableCells);

		auto& allVirtualTableCells = mVegetationVirtualTable->GetVirtualTable();

		// 填充两个GpuGrassClusters
		for (const auto& cell : changedVirtualTableCells) {
			mNeedBakedGpuGrassClusters.emplace_back(
				GpuGrassCluster{ cell.targetGrassClusterRect, cell.cahce->userData.grassBladeBufferIndex }
			);
		}

		for (const auto& tableRow : allVirtualTableCells) {
			for (const auto& cell : tableRow) {
				mVisibleGpuGrassClusters.emplace_back(
					GpuGrassCluster{ cell.targetGrassClusterRect, cell.cahce->userData.grassBladeBufferIndex }
				);
			}
		}
	}

	void VegetationSystem::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		/*
		* 生成ClumpMap
		*/
		renderGraph.AddPass(
			"GenerateClumpMap",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _ClumpParametersBufferDesc{};
				_ClumpParametersBufferDesc.stride = sizeof(ClumpParameter);
				_ClumpParametersBufferDesc.size = sizeof(ClumpParameter) * mClumpParameters.size();
				_ClumpParametersBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_ClumpParametersBufferDesc.aliased = false;
				builder.DeclareBuffer("ClumpParametersBuffer", _ClumpParametersBufferDesc);
				builder.WriteCopyDstBuffer("ClumpParametersBuffer");

				NewTextureProperties _ClumpMapProperties{};
				_ClumpMapProperties.width = smClumpMapSize;
				_ClumpMapProperties.height = smClumpMapSize;
				_ClumpMapProperties.mipLevals = 1u;
				_ClumpMapProperties.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				builder.DeclareTexture("ClumpMap", _ClumpMapProperties);
				builder.WriteTexture("ClumpMap");

				shaderManger.CreateComputeShader("GenerateClumpMap",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/GenerateClumpMap.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* clumpParametersBuffer = resourceStorage->GetResourceByName("ClumpParametersBuffer")->GetBuffer();
				auto* clumpMap = resourceStorage->GetResourceByName("ClumpMap")->GetTexture();

				mGenerateClumpMapPassData.clumpMapSize = Math::Vector2{ smClumpMapSize, smClumpMapSize };
				mGenerateClumpMapPassData.numClumps = mClumpParameters.size();
				mGenerateClumpMapPassData.clumpMapIndex = clumpMap->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::GenerateClumpMapPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mGenerateClumpMapPassData, sizeof(VegetationSystem::GenerateClumpMapPassData));

				commandBuffer.UploadBufferRegion(clumpParametersBuffer, 0u, mClumpParameters.data(), sizeof(ClumpParameter) * mClumpParameters.size());

				uint32_t threadGroupCountX = (smClumpMapSize + smGenerateClumpMapThreadSizeInGroup - 1) / smGenerateClumpMapThreadSizeInGroup;
				uint32_t threadGroupCountY = threadGroupCountX;
				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("GenerateClumpMap");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			}
		);

		/*
		* 草点的实时烘焙与剔除
		*/
		renderGraph.AddPass(
			"BakeAndCullGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);
				
				builder.ReadBuffer("ClumpParametersBuffer", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("ClumpMap", ShaderAccessFlag::NonPixelShader);

				builder.ReadTexture("TerrainHeightMap", ShaderAccessFlag::NonPixelShader);	// 由TerrainSystem导入
				builder.ReadTexture("GrassLayerMask", ShaderAccessFlag::NonPixelShader);	// 由VegetationDataCache导入
				builder.WriteBuffer("BakedGrassBladeList");									// 由VegetationDataCache导入

				NewBufferProperties _GrassClusterListProperties{};
				_GrassClusterListProperties.stride = sizeof(GpuGrassCluster);
				_GrassClusterListProperties.size = smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * _GrassClusterListProperties.stride;
				_GrassClusterListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassClusterListProperties.aliased = false;
				builder.DeclareBuffer("GrassClusterList", _GrassClusterListProperties);
				builder.WriteCopyDstBuffer("GrassClusterList");

				// 8 MB(LOD 0)
				NewBufferProperties _VisibleLOD0GrassBladeIndexListProperties{};
				_VisibleLOD0GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD0GrassBladeIndexListProperties.size = smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis * smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis / 2u;
				_VisibleLOD0GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleLOD0GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("VisibleLOD0GrassBladeIndexList", _VisibleLOD0GrassBladeIndexListProperties);
				builder.WriteBuffer("VisibleLOD0GrassBladeIndexList");

				// 8MB(LOD 1)
				NewBufferProperties _VisibleLOD1GrassBladeIndexListProperties{};
				_VisibleLOD1GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD1GrassBladeIndexListProperties.size = smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis * smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis / 2u;
				_VisibleLOD1GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleLOD1GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("VisibleLOD1GrassBladeIndexList", _VisibleLOD1GrassBladeIndexListProperties);
				builder.WriteBuffer("VisibleLOD1GrassBladeIndexList");

				shaderManger.CreateComputeShader("BakeGrassBlade",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/BakeGrassBlade.hlsl";
					});

				shaderManger.CreateComputeShader("CullGrassBlade",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/CullGrassBlade.hlsl";
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* clumpParametersBuffer = resourceStorage->GetResourceByName("ClumpParametersBuffer")->GetBuffer();
				auto* clumpMap = resourceStorage->GetResourceByName("ClumpMap")->GetTexture();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* grassLayerMask = resourceStorage->GetResourceByName("GrassLayerMask")->GetTexture();
				auto* bakedGrassBladeList = resourceStorage->GetResourceByName("BakedGrassBladeList")->GetBuffer();
				auto* grassClusterList = resourceStorage->GetResourceByName("GrassClusterList")->GetBuffer();
				auto* visibleLOD0GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD0GrassBladeIndexList")->GetBuffer();
				auto* visibleLOD1GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD1GrassBladeIndexList")->GetBuffer();

				if (!mNeedBakedGpuGrassClusters.empty()) {

					mBakeGrassBladePassData.terrainWorldMeterSize = mTerrainSystem->worldMeterSize;
					mBakeGrassBladePassData.terrainHeightMapIndex = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.heightScale = mTerrainSystem->worldHeightScale;
					mBakeGrassBladePassData.clumpMapIndex = clumpMap->GetSRDescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.clumpMapScale = 0.1f;
					mBakeGrassBladePassData.clumpParameterBufferIndex = clumpParametersBuffer->GetSRDescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.clumpParameterNums = mClumpParameters.size();
					mBakeGrassBladePassData.needBakedGrassClusterListIndex = grassClusterList->GetUADescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.grassLayerMaskIndex = grassLayerMask->GetSRDescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetUADescriptor()->GetHeapIndex();
					mBakeGrassBladePassData.grassResolution = smMaxGrassBladeCountPerAxis;

					// 执行草点(Grass Blade)烘焙工作
					auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::BakeGrassBladePassData), 256u);
					memcpy(passDataAlloc.cpuAddress, &mBakeGrassBladePassData, sizeof(VegetationSystem::BakeGrassBladePassData));

					// 上传NeedBakedGrassCluster
					commandBuffer.UploadBufferRegion(grassClusterList, 0u, mNeedBakedGpuGrassClusters.data(), sizeof(GpuGrassCluster) * mNeedBakedGpuGrassClusters.size());

					uint32_t threadGroupCountY = (smMaxGrassBladeCountPerAxis + smBakeGrassBladeThreadSizeInGroup - 1) / smBakeGrassBladeThreadSizeInGroup;
					uint32_t threadGroupCountZ = threadGroupCountY;

					commandBuffer.SetComputeRootSignature();
					commandBuffer.SetComputePipelineState("BakeGrassBlade");
					commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
					commandBuffer.Dispatch(mNeedBakedGpuGrassClusters.size(), threadGroupCountY, threadGroupCountZ);
				}

				// 执行剔除工作
				mCullGrassBladePassData.visibleGrassClusterListIndex = grassClusterList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetSRDescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD0GrassBladeIndexListIndex = visibleLOD0GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD1GrassBladeIndexListIndex = visibleLOD1GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.grassResolution = smMaxGrassBladeCountPerAxis;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::CullGrassBladePassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mCullGrassBladePassData, sizeof(VegetationSystem::CullGrassBladePassData));

				commandBuffer.UploadBufferRegion(grassClusterList, 0, mVisibleGpuGrassClusters.data(), sizeof(GpuGrassCluster)* mVisibleGpuGrassClusters.size());

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(bakedGrassBladeList, GHL::EResourceState::NonPixelShaderAccess);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.ClearCounterBuffer(visibleLOD0GrassBladeIndexList, 0u);
				commandBuffer.ClearCounterBuffer(visibleLOD1GrassBladeIndexList, 0u);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				uint32_t threadGroupCountY = (smMaxGrassBladeCountPerAxis + smCullGrassBladeThreadSizeInGroup - 1) / smCullGrassBladeThreadSizeInGroup;
				uint32_t threadGroupCountZ = threadGroupCountY;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullGrassBlade");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(mVisibleGpuGrassClusters.size(), threadGroupCountY, threadGroupCountZ);
			});

		/*
		* 草点渲染
		*/
		renderGraph.AddPass(
			"RenderGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("NormalRoughness");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteDepthStencil("DepthStencil");
				builder.ReadBuffer("VisibleLOD0GrassBladeIndexList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("VisibleLOD1GrassBladeIndexList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("BakedGrassBladeList", ShaderAccessFlag::NonPixelShader);			// 由VegetationDataCache导入

				NewBufferProperties _GrassBladeRendererIndirectArgsProperties{};
				_GrassBladeRendererIndirectArgsProperties.stride = sizeof(IndirectDraw);
				_GrassBladeRendererIndirectArgsProperties.size = 2u * _GrassBladeRendererIndirectArgsProperties.stride;	// LOD0 LOD1
				_GrassBladeRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_GrassBladeRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("GrassBladeRendererIndirectArgs", _GrassBladeRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("GrassBladeRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("RenderGrassBlade",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/RenderGrassBlade.hlsl";
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

				commandSignatureManger.CreateCommandSignature("RenderGrassBlade",
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
				auto* bakedGrassBladeList = resourceStorage->GetResourceByName("BakedGrassBladeList")->GetBuffer();
				auto* visibleLOD0GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD0GrassBladeIndexList")->GetBuffer();
				auto* visibleLOD1GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD1GrassBladeIndexList")->GetBuffer();
				auto* grassBladeRendererIndirectArgs = resourceStorage->GetResourceByName("GrassBladeRendererIndirectArgs")->GetBuffer();

				mRenderLOD0GrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD0GrassBladePassData.visibleGrassBladeIndexListIndex = visibleLOD0GrassBladeIndexList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD0GrassBladePassData.grassVertexBufferIndex = mLOD0GrassVertexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD0GrassBladePassData.grassIndexBufferIndex = mLOD0GrassVertexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD0GrassBladePassData.currLODIndex = 0;

				mRenderLOD1GrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.visibleGrassBladeIndexListIndex = visibleLOD1GrassBladeIndexList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.grassVertexBufferIndex = mLOD1GrassVertexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.grassIndexBufferIndex = mLOD1GrassVertexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.currLODIndex = 1;

				auto passDataAllocLOD0 = dynamicAllocator->Allocate(sizeof(VegetationSystem::RenderGrassBladePassData), 256u);
				auto passDataAllocLOD1 = dynamicAllocator->Allocate(sizeof(VegetationSystem::RenderGrassBladePassData), 256u);
				memcpy(passDataAllocLOD0.cpuAddress, &mRenderLOD0GrassBladePassData, sizeof(VegetationSystem::RenderGrassBladePassData));
				memcpy(passDataAllocLOD1.cpuAddress, &mRenderLOD1GrassBladePassData, sizeof(VegetationSystem::RenderGrassBladePassData));

				std::vector<IndirectDraw> mutipleIndirectDraw(2, IndirectDraw{});
				mutipleIndirectDraw[0].frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				mutipleIndirectDraw[0].lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				mutipleIndirectDraw[0].passDataAddress = passDataAllocLOD0.gpuAddress;
				mutipleIndirectDraw[0].drawArguments.VertexCountPerInstance = mLOD0GrassIndices.size();
				mutipleIndirectDraw[0].drawArguments.InstanceCount = 0u;
				mutipleIndirectDraw[0].drawArguments.StartVertexLocation = 0u;
				mutipleIndirectDraw[0].drawArguments.StartInstanceLocation = 0u;

				mutipleIndirectDraw[1] = mutipleIndirectDraw[0];
				mutipleIndirectDraw[1].passDataAddress = passDataAllocLOD1.gpuAddress;
				mutipleIndirectDraw[1].drawArguments.VertexCountPerInstance = mLOD1GrassIndices.size();
				mutipleIndirectDraw[1].drawArguments.InstanceCount = 0u;
				mutipleIndirectDraw[1].drawArguments.StartVertexLocation = 0u;
				mutipleIndirectDraw[1].drawArguments.StartInstanceLocation = 0u;

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopySource);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopySource);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopySource);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.ClearCounterBuffer(grassBladeRendererIndirectArgs, 2u);
				commandBuffer.UploadBufferRegion(grassBladeRendererIndirectArgs, 0u, mutipleIndirectDraw.data(), mutipleIndirectDraw.size() * sizeof(IndirectDraw));
				// Copy LOD0 Instance Count
				commandBuffer.CopyBufferRegion(grassBladeRendererIndirectArgs,
					sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(UINT),
					visibleLOD0GrassBladeIndexList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				// Copy LOD1 Instance Count
				commandBuffer.CopyBufferRegion(grassBladeRendererIndirectArgs,
					sizeof(IndirectDraw) + sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(UINT),
					visibleLOD1GrassBladeIndexList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
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
				commandBuffer.SetGraphicsPipelineState("RenderGrassBlade");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("RenderGrassBlade", grassBladeRendererIndirectArgs, 2u);
			});
	}

	void VegetationSystem::FillMiscData() {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		// Fill Clump Parameters
		{
			mClumpParameters.resize(3u);

			mClumpParameters[0];
			mClumpParameters[0].pullToCentre = 0;
			mClumpParameters[0].pointInSameDirection = 0.6;
			mClumpParameters[0].baseHeight = 0.6;
			mClumpParameters[0].heightRandom = 0.1;
			mClumpParameters[0].baseWidth = 0.03;
			mClumpParameters[0].widthRandom = 0.01;
			mClumpParameters[0].baseTilt = 0.93;
			mClumpParameters[0].tiltRandom = 0.07;
			mClumpParameters[0].baseBend = 0.05;
			mClumpParameters[0].bendRandom = 0.01;

			mClumpParameters[1];
			mClumpParameters[1].pullToCentre = 0;
			mClumpParameters[1].pointInSameDirection = 0.2;
			mClumpParameters[1].baseHeight = 0.5;
			mClumpParameters[1].heightRandom = 0.1;
			mClumpParameters[1].baseWidth = 0.03;
			mClumpParameters[1].widthRandom = 0.01;
			mClumpParameters[1].baseTilt = 0.93;
			mClumpParameters[1].tiltRandom = 0.07;
			mClumpParameters[1].baseBend = 0.08;
			mClumpParameters[1].bendRandom = 0.01;

			mClumpParameters[2];
			mClumpParameters[2].pullToCentre = 0;
			mClumpParameters[2].pointInSameDirection = 0.2;
			mClumpParameters[2].baseHeight = 0.8;
			mClumpParameters[2].heightRandom = 0.1;
			mClumpParameters[2].baseWidth = 0.03;
			mClumpParameters[2].widthRandom = 0.01;
			mClumpParameters[2].baseTilt = 0.93;
			mClumpParameters[2].tiltRandom = 0.07;
			mClumpParameters[2].baseBend = 0.08;
			mClumpParameters[2].bendRandom = 0.01;
		}

		// Load LOD0 Vertices & Indices Data
		{

		}

		// Load LOD1 Vertices & Indices Data
		{

		}

		// Load Grass Albedo & Normal
		{

		}
	}
}