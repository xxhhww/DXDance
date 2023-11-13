#include "Renderer/VegetationSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/TerrainSystem.h"
#include "Renderer/FixedTextureHelper.h"

namespace Renderer {

	struct IndirectDispatch {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS lightDataAddress;
		D3D12_DISPATCH_ARGUMENTS  dispatchArguments;
		uint32_t pad1;
	};

	struct IndirectDraw {
	public:
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_DRAW_ARGUMENTS		 drawArguments;
	};

	VegetationSystem::VegetationSystem(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine)
	, mTerrainSystem(renderEngine->mTerrainSystem.get()) {}

	void VegetationSystem::Initialize(RenderEngine* renderEngine) {
		FillMiscData();

		TerrainSystem* terrainSystem = mRenderEngine->mTerrainSystem.get();

		mDataCacher = std::make_unique<VegetationDataCache>(mRenderEngine);
		mDataCacher->ConfigureGrassClusterCache(smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * 1.5f, smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis);
		mDataCacher->ConfigureGrassMaskCache("E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/GrassLayer.png", 16, 256);
		mVegetationVirtualTable = std::make_unique<VegetationVirtualTable>(mDataCacher.get(), smVisibleGrassClusterCountPerAxis, smGrassClusterMeterSize, terrainSystem->worldMeterSize);
	}

	/*
	* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出，由RenderEngine的Update函数调用
	*/
	void VegetationSystem::Update(const Math::Vector2& cameraPosition) {
		mNeedBakedGpuGrassClusters.clear();
		mNeedCulledGpuGrassClusters.clear();

		std::vector<VegetationVirtualTable::VirtualTableCell*> changedVirtualTableCells;
		mVegetationVirtualTable->Update(cameraPosition, changedVirtualTableCells);

		auto& allVirtualTableCells = mVegetationVirtualTable->GetVirtualTable();

		// 获取需要重新烘焙的GrassCluster
		/*
		for (const auto& cell : changedVirtualTableCells) {
			mNeedBakedGpuGrassClusters.emplace_back(
				GpuGrassCluster{ cell->targetGrassClusterRect, cell->cahce->userData.grassBladeBufferIndex }
			);
		}
		*/

		// 获取可见的GrassCluster(CPU上做裁剪)
		for (const auto& tableRow : allVirtualTableCells) {
			for (const auto& cell : tableRow) {
				mNeedBakedGpuGrassClusters.emplace_back(
					GpuGrassCluster{ cell->targetGrassClusterRect, cell->cahce->userData.grassBladeBufferIndex }
				);

				mNeedCulledGpuGrassClusters.emplace_back(
					GpuGrassCluster{ cell->targetGrassClusterRect, cell->cahce->userData.grassBladeBufferIndex }
				);
			}
		}
	}

	void VegetationSystem::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();

		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		/*
		* 生成ClumpMap
		*/
		renderGraph->AddPass(
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
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/ClumpMapGenerator.hlsl";
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
		* GrassCluster的烘焙与剔除
		*/
		renderGraph->AddPass(
			"BakeAndCullGrassCluster",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("ClumpParametersBuffer", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("ClumpMap", ShaderAccessFlag::NonPixelShader);

				// builder.ReadTexture("TerrainHeightMap", ShaderAccessFlag::NonPixelShader);	// 由TerrainSystem导入
				// builder.ReadTexture("MinMaxHeightMap", ShaderAccessFlag::NonPixelShader);	// 由TerrainSystem导入
				// builder.ReadTexture("GrassLayerMask", ShaderAccessFlag::NonPixelShader);	// 由VegetationDataCache导入
				builder.WriteBuffer("BakedGrassBladeList");									// 由VegetationDataCache导入

				NewBufferProperties _GrassClusterListProperties{};
				_GrassClusterListProperties.stride = sizeof(GpuGrassCluster);
				_GrassClusterListProperties.size = smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * _GrassClusterListProperties.stride;
				_GrassClusterListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassClusterListProperties.aliased = false;
				builder.DeclareBuffer("GrassClusterList", _GrassClusterListProperties);
				builder.WriteBuffer("GrassClusterList");

				NewBufferProperties _VisibleGrassClusterListProperties{};
				_VisibleGrassClusterListProperties.stride = sizeof(GpuGrassCluster);
				_VisibleGrassClusterListProperties.size = smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * _VisibleGrassClusterListProperties.stride;
				_VisibleGrassClusterListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleGrassClusterListProperties.aliased = false;
				builder.DeclareBuffer("VisibleGrassClusterList", _VisibleGrassClusterListProperties);
				builder.WriteBuffer("VisibleGrassClusterList");

				shaderManger.CreateComputeShader("BakeGrassBlade",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/BakeGrassBlade.hlsl";
					});

				shaderManger.CreateComputeShader("CullGrassCluster",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/CullGrassCluster.hlsl";
					});

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* clumpParametersBuffer = resourceStorage->GetResourceByName("ClumpParametersBuffer")->GetBuffer();
				auto* clumpMap = resourceStorage->GetResourceByName("ClumpMap")->GetTexture();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* minmaxHeightMap = resourceStorage->GetResourceByName("MinMaxHeightMap")->GetTexture();
				auto* grassLayerMask = resourceStorage->GetResourceByName("GrassLayerMask")->GetTexture();
				auto* bakedGrassBladeList = resourceStorage->GetResourceByName("BakedGrassBladeList")->GetBuffer();
				auto* grassClusterList = resourceStorage->GetResourceByName("GrassClusterList")->GetBuffer();
				auto* visibleGrassClusterList = resourceStorage->GetResourceByName("VisibleGrassClusterList")->GetBuffer();

				// 执行GrassCluster的烘焙工作
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
					{
						auto barrierBatch = GHL::ResourceBarrierBatch{};
						barrierBatch += commandBuffer.TransitionImmediately(grassClusterList, GHL::EResourceState::CopyDestination);
						commandBuffer.FlushResourceBarrier(barrierBatch);
					}
					commandBuffer.UploadBufferRegion(grassClusterList, 0u, mNeedBakedGpuGrassClusters.data(), sizeof(GpuGrassCluster) * mNeedBakedGpuGrassClusters.size());

					uint32_t threadGroupCountY = (smMaxGrassBladeCountPerAxis + smBakeGrassBladeThreadSizeInGroup - 1) / smBakeGrassBladeThreadSizeInGroup;
					uint32_t threadGroupCountZ = threadGroupCountY;

					commandBuffer.SetComputeRootSignature();
					commandBuffer.SetComputePipelineState("BakeGrassBlade");
					commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
					commandBuffer.Dispatch(mNeedBakedGpuGrassClusters.size(), threadGroupCountY, threadGroupCountZ);
				}

				// 执行GrassCluster的剔除工作
				mCullGrassClusterPassData.needCullGrassClusterListIndex = grassClusterList->GetUADescriptor()->GetHeapIndex();
				mCullGrassClusterPassData.visibleGrassClusterListIndex = visibleGrassClusterList->GetUADescriptor()->GetHeapIndex();
				mCullGrassClusterPassData.minMaxHeightMapIndex = minmaxHeightMap->GetSRDescriptor()->GetHeapIndex();
				mCullGrassClusterPassData.terrainWorldMeterSize = mTerrainSystem->worldMeterSize;
				mCullGrassClusterPassData.terrainHeightScale = mTerrainSystem->worldHeightScale;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::CullGrassClusterPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mCullGrassClusterPassData, sizeof(VegetationSystem::CullGrassClusterPassData));

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(grassClusterList, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(visibleGrassClusterList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}
				commandBuffer.UploadBufferRegion(grassClusterList, 0u, mNeedCulledGpuGrassClusters.data(), mNeedCulledGpuGrassClusters.size() * sizeof(GpuGrassCluster));
				commandBuffer.ClearCounterBuffer(visibleGrassClusterList, 0u);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(visibleGrassClusterList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}
				uint32_t threadGroupCountX = (mNeedCulledGpuGrassClusters.size() + smCullGrassClusterThreadSizeInGroup - 1) / smCullGrassClusterThreadSizeInGroup;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullGrassCluster");
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, 1u, 1u);
			});

		/*
		* 草点的实时烘焙与剔除
		*/
		renderGraph->AddPass(
			"CullGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("BakedGrassBladeList", ShaderAccessFlag::NonPixelShader);		// 由VegetationDataCache导入
				builder.ReadBuffer("VisibleGrassClusterList", ShaderAccessFlag::NonPixelShader);

				// 8 MB(LOD 0)
				NewBufferProperties _VisibleLOD0GrassBladeIndexListProperties{};
				_VisibleLOD0GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD0GrassBladeIndexListProperties.size = _VisibleLOD0GrassBladeIndexListProperties.stride * smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis * smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis / 2u;
				_VisibleLOD0GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleLOD0GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("VisibleLOD0GrassBladeIndexList", _VisibleLOD0GrassBladeIndexListProperties);
				builder.WriteBuffer("VisibleLOD0GrassBladeIndexList");

				// 8MB(LOD 1)
				NewBufferProperties _VisibleLOD1GrassBladeIndexListProperties{};
				_VisibleLOD1GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD1GrassBladeIndexListProperties.size = _VisibleLOD1GrassBladeIndexListProperties.stride * smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis * smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis / 2u;
				_VisibleLOD1GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleLOD1GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("VisibleLOD1GrassBladeIndexList", _VisibleLOD1GrassBladeIndexListProperties);
				builder.WriteBuffer("VisibleLOD1GrassBladeIndexList");

				NewBufferProperties _CullGrassBladeIndirectArgsProperties{};
				_CullGrassBladeIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_CullGrassBladeIndirectArgsProperties.size = sizeof(IndirectDispatch);
				_CullGrassBladeIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_CullGrassBladeIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("CullGrassBladeIndirectArgs", _CullGrassBladeIndirectArgsProperties);
				builder.WriteCopyDstBuffer("CullGrassBladeIndirectArgs");

				shaderManger.CreateComputeShader("CullGrassBlade",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/CullGrassBlade.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("CullGrassBlade",
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

				auto* bakedGrassBladeList = resourceStorage->GetResourceByName("BakedGrassBladeList")->GetBuffer();
				auto* visibleGrassClusterList = resourceStorage->GetResourceByName("VisibleGrassClusterList")->GetBuffer();
				auto* visibleLOD0GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD0GrassBladeIndexList")->GetBuffer();
				auto* visibleLOD1GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD1GrassBladeIndexList")->GetBuffer();
				auto* cullGrassBladeIndirectArgs = resourceStorage->GetResourceByName("CullGrassBladeIndirectArgs")->GetBuffer();

				// 执行剔除工作
				mCullGrassBladePassData.visibleGrassClusterListIndex = visibleGrassClusterList->GetSRDescriptor()->GetHeapIndex();
				mCullGrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetSRDescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD0GrassBladeIndexListIndex = visibleLOD0GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD1GrassBladeIndexListIndex = visibleLOD1GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.grassResolution = smMaxGrassBladeCountPerAxis;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::CullGrassBladePassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mCullGrassBladePassData, sizeof(VegetationSystem::CullGrassBladePassData));

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
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
					barrierBatch += commandBuffer.TransitionImmediately(visibleGrassClusterList->GetCounterBuffer(), GHL::EResourceState::CopySource);
					barrierBatch += commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				uint32_t threadGroupCountY = (smMaxGrassBladeCountPerAxis + smCullGrassBladeThreadSizeInGroup - 1) / smCullGrassBladeThreadSizeInGroup;
				uint32_t threadGroupCountZ = threadGroupCountY;

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = 0;
				indirectDispatch.dispatchArguments.ThreadGroupCountY = threadGroupCountY;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = threadGroupCountZ;

				commandBuffer.UploadBufferRegion(cullGrassBladeIndirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.CopyBufferRegion(cullGrassBladeIndirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u, visibleGrassClusterList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(cullGrassBladeIndirectArgs, 1u);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("CullGrassBlade");
				commandBuffer.ExecuteIndirect("CullGrassBlade", cullGrassBladeIndirectArgs, 1u);
			});

		/*
		* 草点渲染
		*/
		renderGraph->AddPass(
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
				mRenderLOD0GrassBladePassData.grassIndexBufferIndex = mLOD0GrassIndexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD0GrassBladePassData.currLODIndex = 0;

				mRenderLOD1GrassBladePassData.bakedGrassBladeListIndex = bakedGrassBladeList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.visibleGrassBladeIndexListIndex = visibleLOD1GrassBladeIndexList->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.grassVertexBufferIndex = mLOD1GrassVertexBuffer->GetSRDescriptor()->GetHeapIndex();
				mRenderLOD1GrassBladePassData.grassIndexBufferIndex = mLOD1GrassIndexBuffer->GetSRDescriptor()->GetHeapIndex();
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
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopySource);
					barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopySource);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}

				commandBuffer.UploadBufferRegion(grassBladeRendererIndirectArgs, 0u, mutipleIndirectDraw.data(), mutipleIndirectDraw.size() * sizeof(IndirectDraw));
				// Copy LOD0 Instance Count
				commandBuffer.CopyBufferRegion(grassBladeRendererIndirectArgs,
					sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(UINT),
					visibleLOD0GrassBladeIndexList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				// Copy LOD1 Instance Count
				commandBuffer.CopyBufferRegion(grassBladeRendererIndirectArgs,
					sizeof(IndirectDraw) + sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(UINT),
					visibleLOD1GrassBladeIndexList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(grassBladeRendererIndirectArgs, 2u);

				{
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(grassBladeRendererIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
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
		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

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
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.45f, 0.22f }, 0.309f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.55f, 0.0f  }, 0.0f  , 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.45f, 0.0f  }, 0.0f  , 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.55f, 0.22f }, 0.309f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.55f, 0.38f }, 0.472f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.45f, 0.38f }, 0.472f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.54f, 0.54f }, 0.601f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.46f, 0.54f }, 0.601f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.54f, 0.67f }, 0.716f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.46f, 0.67f }, 0.716f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.53f, 0.78f }, 0.817f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.47f, 0.78f }, 0.817f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.53f, 0.89f }, 0.913f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.48f, 0.89f }, 0.913f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.5f, 1.0f   }, 1.0f  , 0.498f);

			mLOD0GrassIndices = {
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

			// 创建VertexBuffer / IndexBuffer
			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(GrassVertexAttribute);
			vertexBufferDesc.size = vertexBufferDesc.stride * mLOD0GrassVertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			vertexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			vertexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess;
			mLOD0GrassVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);
			mLOD0GrassVertexBuffer->SetDebugName("LOD0GrassVertexBuffer");

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * mLOD0GrassIndices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			indexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			indexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess;
			mLOD0GrassIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);
			mLOD0GrassIndexBuffer->SetDebugName("LOD0GrassIndexBuffer");

			// upload vertices
			DSTORAGE_REQUEST request = {};
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.Memory.Source = static_cast<void*>(mLOD0GrassVertices.data());
			request.Source.Memory.Size = mLOD0GrassVertices.size() * sizeof(GrassVertexAttribute);
			request.Destination.Buffer.Resource = mLOD0GrassVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD0GrassVertices.size() * sizeof(GrassVertexAttribute);
			request.UncompressedSize = mLOD0GrassVertices.size() * sizeof(GrassVertexAttribute);
			copyDsQueue->EnqueueRequest(&request);

			// upload indices
			request.Source.Memory.Source = static_cast<void*>(mLOD0GrassIndices.data());
			request.Source.Memory.Size = mLOD0GrassIndices.size() * sizeof(uint32_t);
			request.Destination.Buffer.Resource = mLOD0GrassIndexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD0GrassIndices.size() * sizeof(uint32_t);
			request.UncompressedSize = mLOD0GrassIndices.size() * sizeof(uint32_t);
			copyDsQueue->EnqueueRequest(&request);

			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();
		}

		// Load LOD1 Vertices & Indices Data
		{
			mLOD1GrassVertices = mLOD0GrassVertices;
			mLOD1GrassIndices = mLOD0GrassIndices;

			// 创建VertexBuffer / IndexBuffer
			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(GrassVertexAttribute);
			vertexBufferDesc.size = vertexBufferDesc.stride * mLOD1GrassVertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			vertexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			vertexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess;
			mLOD1GrassVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * mLOD1GrassIndices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			indexBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			indexBufferDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess;
			mLOD1GrassIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);

			// upload vertices
			DSTORAGE_REQUEST request = {};
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.Memory.Source = static_cast<void*>(mLOD1GrassVertices.data());
			request.Source.Memory.Size = mLOD1GrassVertices.size() * sizeof(GrassVertexAttribute);
			request.Destination.Buffer.Resource = mLOD1GrassVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD1GrassVertices.size() * sizeof(GrassVertexAttribute);
			request.UncompressedSize = mLOD1GrassVertices.size() * sizeof(GrassVertexAttribute);
			copyDsQueue->EnqueueRequest(&request);

			// upload indices
			request.Source.Memory.Source = static_cast<void*>(mLOD1GrassIndices.data());
			request.Source.Memory.Size = mLOD1GrassIndices.size() * sizeof(uint32_t);
			request.Destination.Buffer.Resource = mLOD1GrassIndexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD1GrassIndices.size() * sizeof(uint32_t);
			request.UncompressedSize = mLOD1GrassIndices.size() * sizeof(uint32_t);
			copyDsQueue->EnqueueRequest(&request);

			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();
		}

		// Load Grass Albedo & Normal
		{
			// 加载GrassAlbedoMap
			mGrassAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Grass/Grass_Albedo.png");
			resourceStateTracker->StartTracking(mGrassAlbedoMap);

			// 加载GrassNormalMap
			mGrassNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Grass/Grass_Normal.png"
			);
			resourceStateTracker->StartTracking(mGrassNormalMap);
		}
	}
}