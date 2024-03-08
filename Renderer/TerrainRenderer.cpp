#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainPipelinePass.h"
#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"
#include "Renderer/RuntimeVTBackend.h"
#include "Renderer/RuntimeVTAtlas.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"
#include "Tools/StrUtil.h"
#include "Tools/Assert.h"

#include "Math/Int.h"

#include <fstream>

namespace Renderer {

	TerrainRenderer::TerrainRenderer(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) 
	, mQueuedFeedbackReadbacks(mRenderEngine->mFrameTracker->GetMaxSize()) {}
	
	TerrainRenderer::~TerrainRenderer() {}

	void TerrainRenderer::Initialize() {

		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* frameTracker = mRenderEngine->mFrameTracker.get();
		auto* resourceStorage = mRenderEngine->mPipelineResourceStorage;
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		auto& finalOutputDesc = resourceStorage->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();


		std::string dirname = "E:/TerrainOfflineTask/001/Runtime/";

		// 创建TempMappingQueue
		{
			mTempMappingQueue = std::make_unique<GHL::CopyQueue>(device);
			mTempMappingQueue->SetDebugName("TerrainRenderer_TempMappingQueue");
			mTempMappingFence = std::make_unique<GHL::Fence>(device);
			mTempMappingFence->SetDebugName("TerrainRenderer_TempMappingFence");
		}

		// 读取地形数据
		{
			std::ifstream terrainLodDescriptorStream(dirname + "TerrainLodDescriptor.bin", std::ios::binary | std::ios::in);
			Tool::InputMemoryStream terrainLodDescriptorInputMem(terrainLodDescriptorStream);
			size_t terrainLodDescriptorSize = 0u;
			terrainLodDescriptorInputMem.Read(terrainLodDescriptorSize);
			mTerrainLodDescriptors.resize(terrainLodDescriptorSize);
			terrainLodDescriptorInputMem.Read(mTerrainLodDescriptors.data(), sizeof(TerrainLodDescriptor) * terrainLodDescriptorSize);

			std::ifstream terrainNodeDescriptorStream(dirname + "TerrainNodeDescriptor.bin", std::ios::binary | std::ios::in);
			Tool::InputMemoryStream terrainNodeDescriptorInputMem(terrainNodeDescriptorStream);
			size_t terrainNodeDescriptorSize = 0u;
			terrainNodeDescriptorInputMem.Read(terrainNodeDescriptorSize);
			mTerrainNodeDescriptors.resize(terrainNodeDescriptorSize);
			terrainNodeDescriptorInputMem.Read(mTerrainNodeDescriptors.data(), sizeof(TerrainNodeDescriptor) * terrainNodeDescriptorSize);

			mTerrainNodeRuntimeStates.resize(terrainNodeDescriptorSize);

			// Far
			mFarTerrainHeightMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainHeightMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);
			mFarTerrainAlbedoMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainAlbedoMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);
			mFarTerrainNormalMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainNormalMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);

			// FarCache
			mFarTerrainTextureAtlasTileCache = std::make_unique<TerrainTextureAtlasTileCache>(mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);

			// TextureArray
			mNearTerrainAlbedoArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainAlbedoArray.ret");
			mNearTerrainNormalArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainNormalArray.ret");
			mTerrainTextureArrayHeapAllocator = std::make_unique<Renderer::BuddyHeapAllocator>(device, frameTracker);

			// TiledSplatMap
			{
				mTerrainTiledSplatMap = std::make_unique<TerrainTiledTexture>(this, dirname + "TerrainTiledSplatMap.ret");
				const auto& reTextureFileFormat = mTerrainTiledSplatMap->GetReTextureFileFormat();
				const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
				const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

				mTerrainTiledSplatMapHeapAllocator = std::make_unique<BuddyHeapAllocator>(device, frameTracker);
				mTerrainTiledSplatMapTileRuntimeStates.resize(reTextureFileHeader.tileNums);
				mTerrainTiledSplatMapHeapAllocationCache = std::make_unique<TerrainTiledTextureHeapAllocationCache>(mTerrainSetting.smTerrainTiledSplatMapTileCountPerCache, mTerrainTiledSplatMapHeapAllocator.get(), reTextureFileHeader.tileSlicePitch);

				// only use mip 1 of the resource. Subsequent mips provide little additional coverage while complicating lookup arithmetic
				uint32_t subresourceCount = 1;
				// create a maximum size reserved resource
				D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)reTextureFileHeader.dxgiFormat, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
				rd.MipLevels = (UINT16)subresourceCount;
				// Layout must be D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE when creating reserved resources
				rd.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

				// this will only ever be a copy dest
				HRASSERT(device->D3DDevice()->CreateReservedResource(&rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mTerrainTiledSplatMapBackend)));
				mTerrainTiledSplatMapBackend->SetName(L"TerrainTiledSplatMapBackend");
				D3D12_PACKED_MIP_INFO packedMipInfo; // unused, for now
				D3D12_TILE_SHAPE tileShape; // unused, for now
				UINT numAtlasTiles = 0;
				device->D3DDevice()->GetResourceTiling(mTerrainTiledSplatMapBackend.Get(), &numAtlasTiles, &packedMipInfo, &tileShape, &subresourceCount, 0, &mTerrainTiledSplatMapBackendTiling);
				numAtlasTiles = mTerrainSetting.smTerrainTiledSplatMapTileCountPerCache;

				// The following depends on the linear assignment order defined by D3D12_REGION_SIZE UseBox = FALSE
				// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_tile_region_size

				// we are updating a single region: all the tiles
				uint32_t numResourceRegions = 1;
				D3D12_TILED_RESOURCE_COORDINATE resourceRegionStartCoordinates{ 0, 0, 0, 0 };
				D3D12_TILE_REGION_SIZE resourceRegionSizes{ numAtlasTiles, FALSE, 0, 0, 0 };

				// we can do this with a single range
				uint32_t numRanges = 1;
				uint32_t tileOffset = 0u;
				std::vector<D3D12_TILE_RANGE_FLAGS>rangeFlags(numRanges, D3D12_TILE_RANGE_FLAG_NONE);
				std::vector<UINT> rangeTileCounts(numRanges, numAtlasTiles);

				mTempMappingQueue->D3DCommandQueue()->UpdateTileMappings(
					mTerrainTiledSplatMapBackend.Get(),
					numResourceRegions,
					&resourceRegionStartCoordinates,
					&resourceRegionSizes,
					mTerrainTiledSplatMapHeapAllocator->GetHeap(0u)->D3DHeap(),
					(UINT)rangeFlags.size(),
					rangeFlags.data(),
					&tileOffset,
					rangeTileCounts.data(),
					D3D12_TILE_MAPPING_FLAG_NONE
				);
				mTempMappingFence->IncrementExpectedValue();
				mTempMappingQueue->SignalFence(*mTempMappingFence.get());
				mTempMappingFence->Wait();
			}

			/*
			// TiledGrassLandMap
			{
				mTerrainTiledGrassLandMap = std::make_unique<TerrainTiledTexture>(this, dirname + "TerrainTiledGrassLandMap.ret");
				const auto& reTextureFileFormat = mTerrainTiledGrassLandMap->GetReTextureFileFormat();
				const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
				const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

				mTerrainTiledGrassLandMapHeapAllocator = std::make_unique<BuddyHeapAllocator>(device, frameTracker);
				mTerrainTiledGrassLandMapTileRuntimeStates.resize(reTextureFileHeader.tileNums);
				mTerrainTiledGrassLandMapHeapAllocationCache = std::make_unique<TerrainTiledTextureHeapAllocationCache>(mTerrainSetting.smTerrainTiledGrassLandMapTileCountPerCache, mTerrainTiledGrassLandMapHeapAllocator.get(), reTextureFileHeader.tileSlicePitch);

				// only use mip 1 of the resource. Subsequent mips provide little additional coverage while complicating lookup arithmetic
				uint32_t subresourceCount = 1;
				// create a maximum size reserved resource
				D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)reTextureFileHeader.dxgiFormat, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
				rd.MipLevels = (UINT16)subresourceCount;
				// Layout must be D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE when creating reserved resources
				rd.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

				// this will only ever be a copy dest
				HRASSERT(device->D3DDevice()->CreateReservedResource(&rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mTerrainTiledSplatMapBackend)));
				mTerrainTiledSplatMapBackend->SetName(L"TerrainTiledSplatMapBackend");
				D3D12_PACKED_MIP_INFO packedMipInfo; // unused, for now
				D3D12_TILE_SHAPE tileShape; // unused, for now
				UINT numAtlasTiles = 0;
				device->D3DDevice()->GetResourceTiling(mTerrainTiledSplatMapBackend.Get(), &numAtlasTiles, &packedMipInfo, &tileShape, &subresourceCount, 0, &mTerrainTiledSplatMapBackendTiling);
				numAtlasTiles = mTerrainSetting.smTerrainTiledSplatMapTileCountPerCache;

				// The following depends on the linear assignment order defined by D3D12_REGION_SIZE UseBox = FALSE
				// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_tile_region_size

				// we are updating a single region: all the tiles
				uint32_t numResourceRegions = 1;
				D3D12_TILED_RESOURCE_COORDINATE resourceRegionStartCoordinates{ 0, 0, 0, 0 };
				D3D12_TILE_REGION_SIZE resourceRegionSizes{ numAtlasTiles, FALSE, 0, 0, 0 };

				// we can do this with a single range
				uint32_t numRanges = 1;
				uint32_t tileOffset = 0u;
				std::vector<D3D12_TILE_RANGE_FLAGS>rangeFlags(numRanges, D3D12_TILE_RANGE_FLAG_NONE);
				std::vector<UINT> rangeTileCounts(numRanges, numAtlasTiles);

				mTempMappingQueue->D3DCommandQueue()->UpdateTileMappings(
					mTerrainTiledSplatMapBackend.Get(),
					numResourceRegions,
					&resourceRegionStartCoordinates,
					&resourceRegionSizes,
					mTerrainTiledSplatMapHeapAllocator->GetHeap(0u)->D3DHeap(),
					(UINT)rangeFlags.size(),
					rangeFlags.data(),
					&tileOffset,
					rangeTileCounts.data(),
					D3D12_TILE_MAPPING_FLAG_NONE
				);
				mTempMappingFence->IncrementExpectedValue();
				mTempMappingQueue->SignalFence(*mTempMappingFence.get());
				mTempMappingFence->Wait();
			}
			*/
		}

		// 创建并初始化GPU对象
		{
			Renderer::BufferDesc _TerrainLodDescriptorBufferDesc{};
			_TerrainLodDescriptorBufferDesc.stride = sizeof(TerrainLodDescriptor);
			_TerrainLodDescriptorBufferDesc.size = _TerrainLodDescriptorBufferDesc.stride * mTerrainLodDescriptors.size();
			_TerrainLodDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainLodDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainLodDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainLodDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mTerrainLodDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainLodDescriptorBufferDesc, descriptorAllocator, nullptr);
			mTerrainLodDescriptorBuffer->SetDebugName("TerrainLodDescriptor");

			renderGraph->ImportResource("TerrainLodDescriptor", mTerrainLodDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainLodDescriptorBuffer);

			Renderer::BufferDesc _TerrainNodeDescriptorBufferDesc{};
			_TerrainNodeDescriptorBufferDesc.stride = sizeof(TerrainNodeDescriptor);
			_TerrainNodeDescriptorBufferDesc.size = _TerrainNodeDescriptorBufferDesc.stride * mTerrainNodeDescriptors.size();
			_TerrainNodeDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainNodeDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainNodeDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainNodeDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
			mTerrainNodeDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainNodeDescriptorBufferDesc, descriptorAllocator, nullptr);
			mTerrainNodeDescriptorBuffer->SetDebugName("TerrainNodeDescriptor");

			renderGraph->ImportResource("TerrainNodeDescriptor", mTerrainNodeDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainNodeDescriptorBuffer);

			TextureDesc _TerrainFeedbackMapDesc{};
			_TerrainFeedbackMapDesc.width = finalOutputDesc.width / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackMapDesc.height = finalOutputDesc.height / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackMapDesc.format = DXGI_FORMAT_R16G16B16A16_UINT;
			_TerrainFeedbackMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;
			_TerrainFeedbackMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mTerrainFeedbackMap = resourceAllocator->Allocate(device, _TerrainFeedbackMapDesc, descriptorAllocator, nullptr);
			mTerrainFeedbackMap->SetDebugName("TerrainFeedback");

			renderGraph->ImportResource("TerrainFeedback", mTerrainFeedbackMap);
			resourceStateTracker->StartTracking(mTerrainFeedbackMap);

			TextureDesc _TerrainFeedbackDepthMapDesc{};
			_TerrainFeedbackDepthMapDesc.width = finalOutputDesc.width / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackDepthMapDesc.height = finalOutputDesc.height / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackDepthMapDesc.format = DXGI_FORMAT_D32_FLOAT;
			_TerrainFeedbackDepthMapDesc.expectedState = GHL::EResourceState::DepthWrite;
			_TerrainFeedbackDepthMapDesc.clearVaule = GHL::DepthStencilClearValue{ 1.0f, 0u };
			mTerrainFeedbackDepthMap = resourceAllocator->Allocate(device, _TerrainFeedbackDepthMapDesc, descriptorAllocator, nullptr);
			mTerrainFeedbackDepthMap->SetDebugName("TerrainFeedbackDepth");

			renderGraph->ImportResource("TerrainFeedbackDepth", mTerrainFeedbackDepthMap);
			resourceStateTracker->StartTracking(mTerrainFeedbackDepthMap);

			BufferDesc _TerrainReadbackBufferDesc{};
			_TerrainReadbackBufferDesc.size = GetRequiredIntermediateSize(mTerrainFeedbackMap->D3DResource(), 0, 1);
			_TerrainReadbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
			_TerrainReadbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_TerrainReadbackBufferDesc.expectedState = _TerrainReadbackBufferDesc.initialState;
			mTerrainFeedbackReadbackBuffers.resize(frameTracker->GetMaxSize());
			for (uint32_t i = 0; i < mTerrainFeedbackReadbackBuffers.size(); i++) {
				mTerrainFeedbackReadbackBuffers[i] = resourceAllocator->Allocate(device, _TerrainReadbackBufferDesc, descriptorAllocator, nullptr);
				mTerrainFeedbackReadbackBuffers[i]->SetDebugName("TerrainFeedbackReadBack" + std::to_string(i));
				renderGraph->ImportResource("TerrainFeedbackReadBack" + std::to_string(i), mTerrainFeedbackReadbackBuffers[i]);
				resourceStateTracker->StartTracking(mTerrainFeedbackReadbackBuffers[i]);
			}

			// 除以2直到smRvtTileCountPerAxisInPage0Level不能再被2整除
			mPageLevelBias = mTerrainSetting.smRvtPageLevelBias;
			mMaxPageLevel = 0u;
			uint32_t tempCount = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			mRuntimeVTPageTables.emplace_back(mMaxPageLevel, mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			while (tempCount != 1 && tempCount % 2 == 0 && mMaxPageLevel < mTerrainSetting.smRvtMaxPageLevel) {
				mMaxPageLevel++;
				tempCount /= 2;

				mRuntimeVTPageTables.emplace_back(mMaxPageLevel, mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			}

			// RuntimeVTPageTableMap
			TextureDesc _RuntimeVTPageTableMapDesc{};
			_RuntimeVTPageTableMapDesc.width = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.height = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.format = DXGI_FORMAT_R8G8B8A8_UINT;
			_RuntimeVTPageTableMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess | GHL::EResourceState::CopySource | GHL::EResourceState::UnorderedAccess | GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::CopyDestination;
			_RuntimeVTPageTableMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, (float)mTerrainSetting.smRvtMaxPageLevel + 1u, 0.0f };
			mRuntimeVTPageTableMap = resourceAllocator->Allocate(device, _RuntimeVTPageTableMapDesc, descriptorAllocator, nullptr);
			mRuntimeVTPageTableMap->SetDebugName("RuntimeVTPageTableMap");
			renderGraph->ImportResource("RuntimeVTPageTableMap", mRuntimeVTPageTableMap);
			resourceStateTracker->StartTracking(mRuntimeVTPageTableMap);

			mRuntimeVTAlbedoAtlas = std::make_unique<Renderer::RuntimeVTAtlas>(this, DXGI_FORMAT_R8G8B8A8_UNORM, "RuntimeVTAlbedoAtlas");
			mRuntimeVTNormalAtlas = std::make_unique<Renderer::RuntimeVTAtlas>(this, DXGI_FORMAT_R8G8B8A8_UNORM, "RuntimeVTNormalAtlas");
			mRuntimeVTAtlasTileCache = std::make_unique<Renderer::RuntimeVTAtlasTileCache>(mTerrainSetting.smRvtTileCountPerAxisInAtlas);
		}

		// 地形后台线程，负责资源调度
		mTerrainBackend = std::make_unique<TerrainBackend>(this, mTerrainSetting, mTerrainLodDescriptors, mTerrainNodeDescriptors, mTerrainNodeRuntimeStates, mTerrainTiledSplatMapTileRuntimeStates);

		// 实时虚拟纹理线程
		mRuntimeVTRealRectChangedCompletedEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mRuntimeVTRealRectChangedCompletedEvent != nullptr, "CreateEvent Failed");
		mRuntimeVTBackend = std::make_unique<RuntimeVTBackend>(this, mTerrainSetting);

		mTerrainPipelinePass = std::make_unique<TerrainPipelinePass>(this);
	}

	void TerrainRenderer::AddPass() {
		mTerrainPipelinePass->AddPass();
	}

	void TerrainRenderer::Update() {
		const auto& rootConstantsPerFrame = mRenderEngine->mPipelineResourceStorage->rootConstantsPerFrame;
		const auto& cameraPosition = mTerrainSetting.smUseRenderCameraDebug ? rootConstantsPerFrame.currentRenderCamera.position : rootConstantsPerFrame.currentEditorCamera.position;

		// 如果是第一帧，需要同步加载地形数据(当前摄像机位置附近 + 最高级LOD)
		static bool smFirstFrame = true;
		if (smFirstFrame) {
			auto GetFixedPosition = [](const Math::Vector2& position, float cellSize) {
				return Math::Int2{
					(int32_t)std::floor(position.x / cellSize + 0.5f) * (int32_t)cellSize,
					(int32_t)std::floor(position.y / cellSize + 0.5f) * (int32_t)cellSize
				};
			};

			Math::Int2 fixedPos0 = GetFixedPosition(Math::Vector2{ cameraPosition.x, cameraPosition.z }, mTerrainSetting.smWorldMeterSizePerTileInPage0Level);
			Math::Int2 runtimeVTRealRectCenter = GetFixedPosition(Math::Vector2{ (float)fixedPos0.x, (float)fixedPos0.y }, mTerrainSetting.smRvtRealRectChangedViewDistance);

			mRuntimeVTRealRect = Math::Vector4{
				(float)runtimeVTRealRectCenter.x - mTerrainSetting.smRvtRectRadius,
				(float)runtimeVTRealRectCenter.y + mTerrainSetting.smRvtRectRadius,
				2.0f * mTerrainSetting.smRvtRectRadius,
				2.0f * mTerrainSetting.smRvtRectRadius
			};

			mTerrainBackend->Preload();
			mRuntimeVTBackend->Preload();

			smFirstFrame = false;
		}
	}

	void TerrainRenderer::OnRuntimeVTRealRectChanged(const Math::Vector4& currRuntimeVTRealRect) {
		const Math::Vector4 prevRuntimeVTRealRect = mRuntimeVTRealRect;

		const Math::Int2 currRuntimeVTRealRectCenter = Math::Int2{
			(int32_t)currRuntimeVTRealRect.x + (int32_t)currRuntimeVTRealRect.z,
			(int32_t)currRuntimeVTRealRect.y - (int32_t)currRuntimeVTRealRect.w
		};

		const Math::Int2 prevRuntimeVTRealRectCenter = Math::Int2{
			(int32_t)prevRuntimeVTRealRect.x + (int32_t)prevRuntimeVTRealRect.z,
			(int32_t)prevRuntimeVTRealRect.y - (int32_t)prevRuntimeVTRealRect.w
		};

		const Math::Int2 runtimeVTRealRectOffsetInPage0Level = (currRuntimeVTRealRectCenter - prevRuntimeVTRealRectCenter) / (int32_t)mTerrainSetting.smWorldMeterSizePerTileInPage0Level;

		mRuntimeVTRealRect = currRuntimeVTRealRect;
		mRuntimeVTRealRectOffsetInPage0Level = runtimeVTRealRectOffsetInPage0Level;

		// 通知RuntimeVTBackend对RealRectChanged事件做处理
		++mRuntimeVTRealRectChangedFlag;

		// 同步等待RuntimeVTBackend完成处理
		::WaitForSingleObject(mRuntimeVTRealRectChangedCompletedEvent, INFINITE);

		// TODO...
		int32_t i = 0;
	}

	bool TerrainRenderer::CheckRuntimeVTRealRectChanged() {
		if (mRuntimeVTRealRectChangedFlag == 1u) {
			--mRuntimeVTRealRectChangedFlag;
			return true;
		}
		return false;
	}

	void TerrainRenderer::SetRuntimeVTRealRectChangedCompletedEvnet() {
		::SetEvent(mRuntimeVTRealRectChangedCompletedEvent); 
	}

}