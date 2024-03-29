#include "Renderer/GrasslandRenderer.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainTextureAtlas.h"

#include "Math/BoundingBox.h"

namespace Renderer {

	struct IndirectDispatch {
	public:
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

	GrasslandRenderer::GrasslandRenderer(RenderEngine* renderEngine, TerrainRenderer* terrainRenderer)
	: mRenderEngine(renderEngine) 
	, mTerrainRenderer(terrainRenderer) {}

	GrasslandRenderer::~GrasslandRenderer() {}

	void GrasslandRenderer::Initialize() {
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* frameTracker = mRenderEngine->mFrameTracker.get();
		auto* resourceStorage = mRenderEngine->mPipelineResourceStorage;
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();
		auto* dstorageQueue = mRenderEngine->mDStorageMemQueue.get();
		auto* dstorageFence = mRenderEngine->mDStorageFence.get();

		// 初始化
		{
			// 计算草地节点的个数
			uint32_t grasslandNodePerAxis = mTerrainSetting.smTerrainMeterSize / mTerrainSetting.smGrasslandNodeMeterSize;
			uint32_t grasslandNodeCount = grasslandNodePerAxis * grasslandNodePerAxis;

			mGrasslandNodeDescriptors.resize(grasslandNodeCount);
			mGrasslandNodeRuntimeStates.resize(grasslandNodeCount);

			mGrasslandLinearBuffer = std::make_unique<GrasslandLinearBuffer>(this, mTerrainSetting.smGrasslandLinearBufferTileCount, sizeof(GrassBladeDescriptor), mTerrainSetting.smGrassBladeCountInSingleNode);
			mGrasslandLinearBufferCache = std::make_unique<GrasslandLinearBufferCache>(mTerrainSetting.smGrasslandLinearBufferTileCount);

			Renderer::BufferDesc _GrasslandNodeDescriptorBufferDesc{};
			_GrasslandNodeDescriptorBufferDesc.stride = sizeof(GrasslandNodeDescriptor);
			_GrasslandNodeDescriptorBufferDesc.size = _GrasslandNodeDescriptorBufferDesc.stride * mGrasslandNodeDescriptors.size();
			_GrasslandNodeDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_GrasslandNodeDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_GrasslandNodeDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_GrasslandNodeDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
			mGrasslandNodeDescriptorsBuffer = resourceAllocator->Allocate(device, _GrasslandNodeDescriptorBufferDesc, descriptorAllocator, nullptr);
			mGrasslandNodeDescriptorsBuffer->SetDebugName("GrasslandNodeDescriptorBuffer");
		}

		// Load LOD0 Vertices & Indices Data
		{
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.45f, 0.22f }, 0.309f, 0.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.55f, 0.0f  }, 0.0f, 1.0f);
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.45f, 0.0f  }, 0.0f, 0.0f);
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
			mLOD0GrassVertices.emplace_back(Math::Vector2{ 0.5f , 1.0f  }, 1.0f, 0.498f);

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
			vertexBufferDesc.stride = sizeof(GrassVertex);
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
			request.Source.Memory.Size = mLOD0GrassVertices.size() * sizeof(GrassVertex);
			request.Destination.Buffer.Resource = mLOD0GrassVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD0GrassVertices.size() * sizeof(GrassVertex);
			request.UncompressedSize = mLOD0GrassVertices.size() * sizeof(GrassVertex);
			dstorageQueue->EnqueueRequest(&request);

			// upload indices
			request.Source.Memory.Source = static_cast<void*>(mLOD0GrassIndices.data());
			request.Source.Memory.Size = mLOD0GrassIndices.size() * sizeof(uint32_t);
			request.Destination.Buffer.Resource = mLOD0GrassIndexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD0GrassIndices.size() * sizeof(uint32_t);
			request.UncompressedSize = mLOD0GrassIndices.size() * sizeof(uint32_t);
			dstorageQueue->EnqueueRequest(&request);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence, dstorageFence->ExpectedValue());
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}

		// Load LOD1 Vertices & Indices Data
		{
			mLOD1GrassVertices = mLOD0GrassVertices;
			mLOD1GrassIndices = mLOD0GrassIndices;

			// 创建VertexBuffer / IndexBuffer
			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(GrassVertex);
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
			request.Source.Memory.Size = mLOD1GrassVertices.size() * sizeof(GrassVertex);
			request.Destination.Buffer.Resource = mLOD1GrassVertexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD1GrassVertices.size() * sizeof(GrassVertex);
			request.UncompressedSize = mLOD1GrassVertices.size() * sizeof(GrassVertex);
			dstorageQueue->EnqueueRequest(&request);

			// upload indices
			request.Source.Memory.Source = static_cast<void*>(mLOD1GrassIndices.data());
			request.Source.Memory.Size = mLOD1GrassIndices.size() * sizeof(uint32_t);
			request.Destination.Buffer.Resource = mLOD1GrassIndexBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = mLOD1GrassIndices.size() * sizeof(uint32_t);
			request.UncompressedSize = mLOD1GrassIndices.size() * sizeof(uint32_t);
			dstorageQueue->EnqueueRequest(&request);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence, dstorageFence->ExpectedValue());
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}

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
	}

	void GrasslandRenderer::Update() {
		mCurrFrameGrasslandNodeRequestTasks.clear();
		mCurrFrameGpuGrasslandNodeRequestTasks.clear();
		mCurrFrameVisibleGrasslandNodes.clear();

		ProduceGrasslandNodeRequest(mCurrFrameGrasslandNodeRequestTasks, mCurrFrameVisibleGrasslandNodes);
		for (const auto& currFrameRequestTask : mCurrFrameGrasslandNodeRequestTasks) {
			GpuGrasslandNodeRequestTask currFrameGpuRequestTask;
			currFrameGpuRequestTask.prevGrasslandNodeIndex = currFrameRequestTask.prevGrasslandNodeIndex;
			currFrameGpuRequestTask.nextGrasslandNodeIndex = currFrameRequestTask.nextGrasslandNodeIndex;
			currFrameGpuRequestTask.tileIndex = currFrameRequestTask.cacheNode->tileIndex;
			mCurrFrameGpuGrasslandNodeRequestTasks.push_back(currFrameGpuRequestTask);
		}
	}

	void GrasslandRenderer::AddPass() {
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		const auto shaderPath = mRenderEngine->smEngineShaderPath;

		/*
		* 生成ClumpMap与上传当前帧数据
		*/
		renderGraph->AddPass(
			"GenerateClumpMap",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _GrasslandNodeRequestTasksProperties{};
				_GrasslandNodeRequestTasksProperties.stride = sizeof(GpuGrasslandNodeRequestTask);
				_GrasslandNodeRequestTasksProperties.size = mTerrainSetting.smGrasslandNodeAroundCount * _GrasslandNodeRequestTasksProperties.stride;
				_GrasslandNodeRequestTasksProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrasslandNodeRequestTasksProperties.aliased = false;
				builder.DeclareBuffer("GrasslandNodeRequestTaskList", _GrasslandNodeRequestTasksProperties);
				builder.WriteCopyDstBuffer("GrasslandNodeRequestTaskList");

				NewBufferProperties _VisibleGrasslandNodeListProperties{};
				_VisibleGrasslandNodeListProperties.stride = sizeof(int32_t);
				_VisibleGrasslandNodeListProperties.size = mTerrainSetting.smGrasslandNodeAroundCount * _VisibleGrasslandNodeListProperties.stride;
				_VisibleGrasslandNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleGrasslandNodeListProperties.aliased = false;
				builder.DeclareBuffer("VisibleGrasslandNodeList", _VisibleGrasslandNodeListProperties);
				builder.WriteCopyDstBuffer("VisibleGrasslandNodeList");

				NewBufferProperties _ClumpParametersBufferDesc{};
				_ClumpParametersBufferDesc.stride = sizeof(GrassClumpParameter);
				_ClumpParametersBufferDesc.size = sizeof(GrassClumpParameter) * mClumpParameters.size();
				_ClumpParametersBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_ClumpParametersBufferDesc.aliased = false;
				builder.DeclareBuffer("ClumpParametersBuffer", _ClumpParametersBufferDesc);
				builder.WriteCopyDstBuffer("ClumpParametersBuffer");

				NewTextureProperties _ClumpMapProperties{};
				_ClumpMapProperties.width = mTerrainSetting.smClumpMapSize;
				_ClumpMapProperties.height = mTerrainSetting.smClumpMapSize;
				_ClumpMapProperties.mipLevals = 1u;
				_ClumpMapProperties.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				builder.DeclareTexture("ClumpMap", _ClumpMapProperties);
				builder.WriteTexture("ClumpMap");

				shaderManger.CreateComputeShader("GenerateClumpMap",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = shaderPath + "TerrainRenderer/GrassClumpMapGenerator.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* grasslandNodeRequestTaskList = resourceStorage->GetResourceByName("GrasslandNodeRequestTaskList")->GetBuffer();
				auto* visibleGrasslandNodeList     = resourceStorage->GetResourceByName("VisibleGrasslandNodeList")->GetBuffer();
				auto* clumpParametersBuffer        = resourceStorage->GetResourceByName("ClumpParametersBuffer")->GetBuffer();
				auto* clumpMap                     = resourceStorage->GetResourceByName("ClumpMap")->GetTexture();

				mGenerateClumpMapPassData.clumpMapSize = Math::Vector2{ mTerrainSetting.smClumpMapSize, mTerrainSetting.smClumpMapSize };
				mGenerateClumpMapPassData.numClumps = mClumpParameters.size();
				mGenerateClumpMapPassData.clumpMapIndex = clumpMap->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::GenerateClumpMapPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mGenerateClumpMapPassData, sizeof(VegetationSystem::GenerateClumpMapPassData));

				commandBuffer.UploadBufferRegion(grasslandNodeRequestTaskList, 0u, mCurrFrameGpuGrasslandNodeRequestTasks.data(), sizeof(GpuGrasslandNodeRequestTask) * mCurrFrameGpuGrasslandNodeRequestTasks.size());
				commandBuffer.UploadBufferRegion(visibleGrasslandNodeList, 0u, mCurrFrameVisibleGrasslandNodes.data(), sizeof(int32_t) * mCurrFrameVisibleGrasslandNodes.size());
				commandBuffer.UploadBufferRegion(clumpParametersBuffer, 0u, mClumpParameters.data(), sizeof(GrassClumpParameter) * mClumpParameters.size());

				uint32_t threadGroupCountX = (mTerrainSetting.smClumpMapSize + smGenerateClumpMapThreadSizeInGroup - 1) / smGenerateClumpMapThreadSizeInGroup;
				uint32_t threadGroupCountY = threadGroupCountX;
				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("GenerateClumpMap");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});

		/*
		* 烘焙GrassBlades
		*/
		renderGraph->AddPass(
			"BakeGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("ClumpParametersBuffer", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("ClumpMap", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("GrasslandNodeRequestTaskList", ShaderAccessFlag::NonPixelShader);

				builder.WriteBuffer("GrasslandNodeDescriptorBuffer");
				builder.WriteBuffer("GrasslandLinearBuffer");

				shaderManger.CreateComputeShader("BakeGrassBlade",
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = shaderPath + "TerrainRenderer/GrassbladeBaker.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* clumpParametersBuffer        = resourceStorage->GetResourceByName("ClumpParametersBuffer")->GetBuffer();
				auto* clumpMap                     = resourceStorage->GetResourceByName("ClumpMap")->GetTexture();
				auto* grasslandNodeRequestTaskList = resourceStorage->GetResourceByName("GrasslandNodeRequestTaskList")->GetBuffer();
				auto* grasslandLinearBuffer        = resourceStorage->GetResourceByName("GrasslandLinearBuffer");
				auto& terrainHeightMapAtlas        = mTerrainRenderer->GetFarTerrainHeightMapAtlas()->GetTextureAtlas();
				auto& terrainNodeDescriptorsBuffer = mTerrainRenderer->GetTerrainNodeDescriptorsBuffer();
				auto& terrainLodDescriptorsBuffer  = mTerrainRenderer->GetTerrainLodDescriptorsBuffer();

				mBakeGrassBladePassData.terrainWorldMeterSize             = Math::Vector2{ mTerrainSetting.smTerrainMeterSize, mTerrainSetting.smTerrainMeterSize };
				mBakeGrassBladePassData.heightScale                       = mTerrainSetting.smTerrainHeightScale;
				mBakeGrassBladePassData.terrainHeightMapAtlasIndex        = terrainHeightMapAtlas->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.terrainNodeDescriptorListIndex    = terrainNodeDescriptorsBuffer->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.terrainLodDescriptorListIndex     = terrainLodDescriptorsBuffer->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.grasslandNodeRequestTaskListIndex = grasslandNodeRequestTaskList->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.grasslandNodeDescriptorListIndex  = mGrasslandNodeDescriptorsBuffer->GetUADescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.grasslandLinearBufferIndex        = grasslandLinearBuffer->GetBuffer()->GetUADescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.grassResolution                   = mTerrainSetting.smGrassBladePerAxisInSingleNode;
				mBakeGrassBladePassData.clumpMapIndex                     = clumpMap->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.clumpParameterBufferIndex         = clumpParametersBuffer->GetSRDescriptor()->GetHeapIndex();
				mBakeGrassBladePassData.clumpParameterNums                = mClumpParameters.size();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(BakeGrassBladePassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mBakeGrassBladePassData, sizeof(BakeGrassBladePassData));

				if (!smInitialized) {
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += commandBuffer.TransitionImmediately(mGrasslandNodeDescriptorsBuffer, GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);
					commandBuffer.UploadBufferRegion(mGrasslandNodeDescriptorsBuffer, 0u, mGrasslandNodeDescriptors.data(), sizeof(GrasslandNodeDescriptor) * mGrasslandNodeDescriptors.size());

					barrierBatch =  commandBuffer.TransitionImmediately(mGrasslandNodeDescriptorsBuffer, GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);
					smInitialized = true;
				}

				uint32_t threadGroupCountY = (mTerrainSetting.smGrassBladePerAxisInSingleNode + smBakeGrassBladeThreadSizeInGroup - 1) / smBakeGrassBladeThreadSizeInGroup;
				uint32_t threadGroupCountZ = threadGroupCountY;

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("BakeGrassBlade");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(mCurrFrameGpuGrasslandNodeRequestTasks.size(), threadGroupCountY, threadGroupCountZ);
			});

		/*
		* 剔除GrassBlade
		*/
		renderGraph->AddPass(
			"CullGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("GrasslandLinearBuffer", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("VisibleGrasslandNodeList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _VisibleLOD0GrassBladeIndexListProperties{};
				_VisibleLOD0GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD0GrassBladeIndexListProperties.size = _VisibleLOD0GrassBladeIndexListProperties.stride * mTerrainSetting.smGrassBladeCountInSingleNode * mTerrainSetting.smGrasslandNodeAroundCount;
				_VisibleLOD0GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_VisibleLOD0GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("VisibleLOD0GrassBladeIndexList", _VisibleLOD0GrassBladeIndexListProperties);
				builder.WriteBuffer("VisibleLOD0GrassBladeIndexList");

				NewBufferProperties _VisibleLOD1GrassBladeIndexListProperties{};
				_VisibleLOD1GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_VisibleLOD1GrassBladeIndexListProperties.size = _VisibleLOD1GrassBladeIndexListProperties.stride * mTerrainSetting.smGrassBladeCountInSingleNode * mTerrainSetting.smGrasslandNodeAroundCount;
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
					[&](ComputeStateProxy& proxy) {
						proxy.csFilepath = shaderPath + "TerrainRenderer/GrassBladeSelector.hlsl";
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

				auto* grasslandLinearBuffer          = resourceStorage->GetResourceByName("GrasslandLinearBuffer")->GetBuffer();
				auto* visibleGrasslandNodeList       = resourceStorage->GetResourceByName("VisibleGrasslandNodeList")->GetBuffer();
				auto* visibleLOD0GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD0GrassBladeIndexList")->GetBuffer();
				auto* visibleLOD1GrassBladeIndexList = resourceStorage->GetResourceByName("VisibleLOD1GrassBladeIndexList")->GetBuffer();
				auto* cullGrassBladeIndirectArgs     = resourceStorage->GetResourceByName("CullGrassBladeIndirectArgs")->GetBuffer();

				mCullGrassBladePassData.visibleGrasslandNodeListIndex = visibleGrasslandNodeList->GetSRDescriptor()->GetHeapIndex();
				mCullGrassBladePassData.grasslandLinearBufferIndex    = grasslandLinearBuffer->GetSRDescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD0GrassBladeIndexListIndex = visibleLOD0GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.visibleLOD1GrassBladeIndexListIndex = visibleLOD1GrassBladeIndexList->GetUADescriptor()->GetHeapIndex();
				mCullGrassBladePassData.grassResolution = mTerrainSetting.smGrassBladePerAxisInSingleNode;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(VegetationSystem::CullGrassBladePassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &mCullGrassBladePassData, sizeof(VegetationSystem::CullGrassBladePassData));

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);
				commandBuffer.ClearCounterBuffer(visibleLOD0GrassBladeIndexList, 0u);
				commandBuffer.ClearCounterBuffer(visibleLOD1GrassBladeIndexList, 0u);

				barrierBatch =  commandBuffer.TransitionImmediately(visibleLOD0GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(visibleLOD1GrassBladeIndexList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(visibleGrasslandNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch += commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				uint32_t threadGroupCountY = (mTerrainSetting.smGrassBladePerAxisInSingleNode + smCullGrassBladeThreadSizeInGroup - 1) / smCullGrassBladeThreadSizeInGroup;
				uint32_t threadGroupCountZ = threadGroupCountY;

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = 0;
				indirectDispatch.dispatchArguments.ThreadGroupCountY = threadGroupCountY;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = threadGroupCountZ;

				commandBuffer.UploadBufferRegion(cullGrassBladeIndirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.CopyBufferRegion(cullGrassBladeIndirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u, visibleGrasslandNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));
				commandBuffer.ClearCounterBuffer(cullGrassBladeIndirectArgs, 1u);

				barrierBatch =  commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(cullGrassBladeIndirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				commandBuffer.FlushResourceBarrier(barrierBatch);

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

				builder.WriteRenderTarget("GBufferAlbedoMetalness");
				builder.WriteRenderTarget("GBufferPositionEmission");
				builder.WriteRenderTarget("GBufferNormalRoughness");
				builder.WriteRenderTarget("GBufferMotionVector");
				builder.WriteRenderTarget("GBufferViewDepth");
				builder.WriteDepthStencil("GBufferDepthStencil");

				builder.ReadBuffer("VisibleLOD0GrassBladeIndexList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("VisibleLOD1GrassBladeIndexList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("BakedGrassBladeList", ShaderAccessFlag::NonPixelShader);			// 由VegetationDataCache导入
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;


			});
			
	}

	void GrasslandRenderer::ProduceGrasslandNodeRequest(std::vector<GrasslandNodeRequestTask>& requestTasks, std::vector<int32_t>& visibleGrasslandNodes) {
		auto* pipelineResourceStorage = mRenderEngine->mPipelineResourceStorage;
		
		auto cameraPosition = mTerrainSetting.smUseRenderCameraDebug ? pipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.position : pipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position;
		
		Math::Vector4 cameraPlanes[6];
		memcpy(cameraPlanes, mTerrainSetting.smUseRenderCameraDebug ? pipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.planes : pipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.planes, sizeof(Math::Vector4) * 6);

		auto GetFixedPos = [](const Math::Vector3& pos, float nodeMeterSize, float terrainMeterSize) {
			// 对pos做偏移
			Math::Vector3 transformedPos = pos;
			transformedPos.x = transformedPos.x + terrainMeterSize / 2.0f;
			transformedPos.y = transformedPos.y;
			transformedPos.z = -transformedPos.z + terrainMeterSize / 2.0f;

			Math::Int2 fixedPos = Math::Int2{
				(int32_t)std::floor(transformedPos.x / nodeMeterSize + 0.5f) * (int32_t)nodeMeterSize,
				(int32_t)std::floor(transformedPos.z / nodeMeterSize + 0.5f) * (int32_t)nodeMeterSize
			};

			return Math::Int2{ fixedPos.x / (int32_t)nodeMeterSize, fixedPos.y / (int32_t)nodeMeterSize };
		};

		Math::Int2 fixedPos = GetFixedPos(cameraPosition, (float)mTerrainSetting.smGrasslandNodeMeterSize, mTerrainSetting.smTerrainMeterSize);
		// 考虑范围为： -smsmTerrainDataRange, smTerrainDataRange 内的地形节点
		for (int32_t yIndex = -mTerrainSetting.smGrasslandNodeBakedRange; yIndex <= mTerrainSetting.smGrasslandNodeBakedRange; yIndex++) {
			for (int32_t xIndex = -mTerrainSetting.smGrasslandNodeBakedRange; xIndex <= mTerrainSetting.smGrasslandNodeBakedRange; xIndex++) {
				int32_t currNodeLocationX = fixedPos.x + xIndex;
				int32_t currNodeLocationY = fixedPos.y + yIndex;

				// 剔除非法边界
				if (currNodeLocationX < 0 || currNodeLocationX >((mTerrainSetting.smTerrainMeterSize / mTerrainSetting.smGrasslandNodeMeterSize) - 1)) {
					continue;
				}
				if (currNodeLocationY < 0 || currNodeLocationY >((mTerrainSetting.smTerrainMeterSize / mTerrainSetting.smGrasslandNodeMeterSize) - 1)) {
					continue;
				}

				// 计算出当前节点的全局索引
				uint32_t nodeCountPerRow = mTerrainSetting.smTerrainMeterSize / mTerrainSetting.smGrasslandNodeMeterSize;
				uint32_t currNodeIndex = currNodeLocationY * nodeCountPerRow + currNodeLocationX;


				// 计算当前GrasslandNode对应的TerrainNode
				const auto& terrainLodDescriptors = mTerrainRenderer->GetTerrainLodDescriptors();
				const auto& terrainNodeDescriptors = mTerrainRenderer->GetTerrainNodeDescriptors();
				const auto& lod0Descriptor = terrainLodDescriptors.at(0);
				uint32_t terrainNodeIndex = lod0Descriptor.nodeStartOffset + currNodeIndex;
				const auto& terrainNodeDescriptor = terrainNodeDescriptors.at(terrainNodeIndex);

				// 计算GrasslandNode的BoundingBox
				Math::Vector3 minPosition = ((float)currNodeLocationX       * mTerrainSetting.smGrasslandNodeMeterSize, terrainNodeDescriptor.minHeight, (float)(currNodeLocationY + 1) * mTerrainSetting.smGrasslandNodeMeterSize);
				Math::Vector3 maxPosition = ((float)(currNodeLocationX + 1) * mTerrainSetting.smGrasslandNodeMeterSize, terrainNodeDescriptor.maxHeight, (float)(currNodeLocationY)     * mTerrainSetting.smGrasslandNodeMeterSize);
				minPosition.x = minPosition.x - mTerrainSetting.smTerrainMeterSize / 2.0f;
				minPosition.z = mTerrainSetting.smTerrainMeterSize / 2.0f - minPosition.z;
				maxPosition.x = maxPosition.x - mTerrainSetting.smTerrainMeterSize / 2.0f;
				maxPosition.z = mTerrainSetting.smTerrainMeterSize / 2.0f - maxPosition.z;

				// 计算是否剔除
				Math::BoundingBox boundingBox;
				boundingBox.minPosition = minPosition;
				boundingBox.maxPosition = maxPosition;
				if (Math::FrustumCull(cameraPlanes, boundingBox)) {
					continue;
				};

				// 保留可见的GrasslandNode索引
				visibleGrasslandNodes.push_back(currNodeIndex);

				// 获取该节点的实时状态
				auto& currNodeRuntimeState = mGrasslandNodeRuntimeStates.at(currNodeIndex);

				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) {
					// 该节点对应的资源正在加载
					continue;
				}
				// 该节点对应的资源已在图集上
				else if (!currNodeRuntimeState.inReadyOut && !currNodeRuntimeState.inQueueOut && !currNodeRuntimeState.inLoadingOut && currNodeRuntimeState.inBuffer) {
					mGrasslandLinearBufferCache->Remove(currNodeRuntimeState.cacheNode);
					mGrasslandLinearBufferCache->AddTail(currNodeRuntimeState.cacheNode);
				}
				else {
					// 创建对应的地形请求任务(但不分配图集元素)
					GrasslandNodeRequestTask requestTask{};
					requestTask.nextGrasslandNodeIndex = currNodeIndex;
					requestTasks.push_back(requestTask);
				}
			}
		}

		// 为剩余请求任务申请atlasNode，并更新节点的实时状态
		for (auto& requestTask : requestTasks) {
			auto* atlasNode = mGrasslandLinearBufferCache->GetHead();
			mGrasslandLinearBufferCache->Remove(atlasNode);

			// 如果该atlasNode已经负载了一个地形节点的资源，做记录
			if (atlasNode->grassNodeIndex != -1) {
				requestTask.prevGrasslandNodeIndex = atlasNode->grassNodeIndex;
			}
			// 使用的atlasNode没有负载地形节点
			else {
				requestTask.prevGrasslandNodeIndex = 65536u;
			}
			requestTask.cacheNode = atlasNode;

			auto& currNodeRuntimeState = mGrasslandNodeRuntimeStates.at(requestTask.nextGrasslandNodeIndex);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevGrasslandNodeIndex != 65536u) {
				auto& prevNodeRuntimeState = mGrasslandNodeRuntimeStates.at(requestTask.prevGrasslandNodeIndex);
				prevNodeRuntimeState.SetInReadyOut();
			}
		}
	}

}