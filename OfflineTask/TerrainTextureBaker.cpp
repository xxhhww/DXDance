#include <DirectStorage/dstorage.h>
#include <fstream>

#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/Misc.h"

#include "OfflineTask/TerrainTextureBaker.h"
#include "Tools/StrUtil.h"
#include "Math/Vector.h"
#include "GHL/Box.h"

namespace OfflineTask {

	void TerrainTextureBaker::Initialize(
		const std::string& heightMapFilepath,
		const std::string& splatMapFilepath,
		const std::string& terrainNormalMapPath,
		const std::string& albedoOutputPath,
		const std::string& normalOutputPath,
		Renderer::RenderEngine* renderEngine) {

		mTerrainNormalMapPath = terrainNormalMapPath;
		mOutputAlbedoMapPath = albedoOutputPath;

		auto* device = renderEngine->mDevice.get();
		mDevice = device;

		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStorage = renderEngine->mPipelineResourceStorage;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();
		auto* dstorageQueue = renderEngine->mDStorageMemQueue.get();
		auto* dstorageFence = renderEngine->mDStorageFence.get();

		auto shaderPath = renderEngine->smEngineShaderPath;

		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
		}

		// Load HeightMap From File
		{
			mTerrainHeightMap = Renderer::FixedTextureHelper::LoadFromFile(device, descriptorAllocator, resourceAllocator, dstorageQueue->GetDStorageQueue(), dstorageFence, heightMapFilepath);
			resourceStateTracker->StartTracking(mTerrainHeightMap);
			resourceStorage->ImportResource("TerrainHeightMap", mTerrainHeightMap);
		}

		const auto terrainHeightMapDesc = mTerrainHeightMap->GetResourceFormat().GetTextureDesc();

		// Load SplatMap From File
		{
			mTerrainSplatMap = Renderer::FixedTextureHelper::LoadFromFile(device, descriptorAllocator, resourceAllocator, dstorageQueue->GetDStorageQueue(), dstorageFence, splatMapFilepath);
			resourceStateTracker->StartTracking(mTerrainSplatMap);
			resourceStorage->ImportResource("TerrainSplatMap", mTerrainSplatMap);
		}

		// 创建TerrainNormalMap 与 ReadbackBuffer
		{
			Renderer::TextureDesc _TerrainNormalMapDesc{};
			_TerrainNormalMapDesc.width = terrainHeightMapDesc.width;
			_TerrainNormalMapDesc.height = terrainHeightMapDesc.height;
			_TerrainNormalMapDesc.mipLevals = 1u;
			_TerrainNormalMapDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_TerrainNormalMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
			_TerrainNormalMapDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::PixelShaderAccess | GHL::EResourceState::CopySource;
			mTerrainNormalMap = resourceAllocator->Allocate(device, _TerrainNormalMapDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mTerrainNormalMap);
			resourceStorage->ImportResource("TerrainNormalMap", mTerrainNormalMap);

			Renderer::BufferDesc _TerrainNormalMapReadbackBufferDesc{};
			_TerrainNormalMapReadbackBufferDesc.size = Math::AlignUp(mTerrainNormalMap->GetResourceFormat().GetSizeInBytes(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			_TerrainNormalMapReadbackBufferDesc.stride = 0u;
			_TerrainNormalMapReadbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
			_TerrainNormalMapReadbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_TerrainNormalMapReadbackBufferDesc.expectedState = GHL::EResourceState::CopyDestination;
			mTerrainNormalMapReadbackBuffer = resourceAllocator->Allocate(device, _TerrainNormalMapReadbackBufferDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mTerrainNormalMapReadbackBuffer);
			resourceStorage->ImportResource("TerrainNormalMapReadbackBuffer", mTerrainNormalMapReadbackBuffer);
		}

		// 创建AlbedoMap 与 ReadbackBuffer
		{
			Renderer::TextureDesc _OutputAlbedoDesc{};
			_OutputAlbedoDesc.width = smVertexCountPerAxis;
			_OutputAlbedoDesc.height = smVertexCountPerAxis;
			_OutputAlbedoDesc.mipLevals = 1u;
			_OutputAlbedoDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_OutputAlbedoDesc.initialState = GHL::EResourceState::RenderTarget;
			_OutputAlbedoDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;
			mOutputAlbedoMap = resourceAllocator->Allocate(device, _OutputAlbedoDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputAlbedoMap);
			resourceStorage->ImportResource("OutputAlbedoMap", mOutputAlbedoMap);

			Renderer::BufferDesc _OutputAlbedoMapReadbackBufferDesc{};
			_OutputAlbedoMapReadbackBufferDesc.size = Math::AlignUp(mOutputAlbedoMap->GetResourceFormat().GetSizeInBytes(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			_OutputAlbedoMapReadbackBufferDesc.stride = 0u;
			_OutputAlbedoMapReadbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
			_OutputAlbedoMapReadbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_OutputAlbedoMapReadbackBufferDesc.expectedState = GHL::EResourceState::CopyDestination;
			mOutputAlbedoMapReadbackBuffer = resourceAllocator->Allocate(device, _OutputAlbedoMapReadbackBufferDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputAlbedoMapReadbackBuffer);
			resourceStorage->ImportResource("OutputAlbedoMapReadbackBuffer", mOutputAlbedoMapReadbackBuffer);
		}

		// 创建NormalMap 与 ReadbackBuffer
		{
			Renderer::TextureDesc _OutputNormalDesc{};
			_OutputNormalDesc.width = smVertexCountPerAxis;
			_OutputNormalDesc.height = smVertexCountPerAxis;
			_OutputNormalDesc.mipLevals = 1u;
			_OutputNormalDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_OutputNormalDesc.initialState = GHL::EResourceState::RenderTarget;
			_OutputNormalDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;
			mOutputNormalMap = resourceAllocator->Allocate(device, _OutputNormalDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputNormalMap);
			resourceStorage->ImportResource("OutputNormalMap", mOutputNormalMap);

			Renderer::BufferDesc _OutputNormalMapReadbackBufferDesc{};
			_OutputNormalMapReadbackBufferDesc.size = Math::AlignUp(mOutputNormalMap->GetResourceFormat().GetSizeInBytes(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			_OutputNormalMapReadbackBufferDesc.stride = 0u;
			_OutputNormalMapReadbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
			_OutputNormalMapReadbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_OutputNormalMapReadbackBufferDesc.expectedState = GHL::EResourceState::CopyDestination;
			mOutputNormalMapReadbackBuffer = resourceAllocator->Allocate(device, _OutputNormalMapReadbackBufferDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputNormalMapReadbackBuffer);
			resourceStorage->ImportResource("OutputNormalMapReadbackBuffer", mOutputNormalMapReadbackBuffer);
		}

		// 创建QuadMesh
		{
			std::vector<Renderer::Vertex> vertices;
			vertices.resize(6u);
			vertices[0].position = Math::Vector3{ 0.0f, 1.0f, 0.0f };
			vertices[0].uv = Math::Vector2{ 0.0f, 1.0f };
			vertices[1].position = Math::Vector3{ 0.0f, 0.0f, 0.0f };
			vertices[1].uv = Math::Vector2{ 0.0f, 0.0f };
			vertices[2].position = Math::Vector3{ 1.0f, 0.0f, 0.0f };
			vertices[2].uv = Math::Vector2{ 1.0f, 0.0f };
			vertices[3].position = Math::Vector3{ 1.0f, 1.0f, 0.0f };
			vertices[3].uv = Math::Vector2{ 1.0f, 1.0f };

			std::vector<uint32_t> indices;
			indices.emplace_back(0u);
			indices.emplace_back(1u);
			indices.emplace_back(2u);
			indices.emplace_back(2u);
			indices.emplace_back(3u);
			indices.emplace_back(0u);

			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(Renderer::Vertex);
			vertexBufferDesc.size = vertexBufferDesc.stride * vertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * indices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);
			mQuadMeshIndexCount = indices.size();

			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(vertices.data()), vertexBufferDesc.size, mQuadMeshVertexBuffer.Get(), 0u);
			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(indices.data()), indexBufferDesc.size, mQuadMeshIndexBuffer.Get(), 0u);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence);
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}

		// 创建Shader
		{
			shaderManger->CreateComputeShader(smBakeTerrainNormalMapSN,
				[&](Renderer::ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainNormalMapBaker.hlsl";
				});

			shaderManger->CreateGraphicsShader(smBakeFarTerrainTextureSN,
				[&](Renderer::GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/FarTerrainTextureBaker.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mOutputAlbedoMap->GetResourceFormat().GetTextureDesc().format,	// OutputAlbedoMap
						mOutputNormalMap->GetResourceFormat().GetTextureDesc().format	// OutputNormalMap
					};
				});
		}
	}

	void TerrainTextureBaker::Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext) {
		auto* dynamicAllocator = renderContext.dynamicAllocator;


		// First Pass Bake Terrain NormalMap
		{
			mBakeTerrainTexturePassData.terrainHeightMapIndex = mTerrainHeightMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.terrainNormalMapIndex = mTerrainNormalMap->GetUADescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.terrainSplatMapIndex = mTerrainSplatMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.mvpMatrix;
			mBakeTerrainTexturePassData.tileOffset;
			mBakeTerrainTexturePassData.blendOffset;
			mBakeTerrainTexturePassData.vertexCountPerAxis = smVertexCountPerAxis;
			mBakeTerrainTexturePassData.vertexSpaceInMeterSize = smVertexSpaceInMeterSize;
			mBakeTerrainTexturePassData.terrainMeterSize = smTerrainMeterSize;
			mBakeTerrainTexturePassData.terrainHeightScale = smTerrainHeightScale;

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(BakeTerrainTexturePassData));
			memcpy(passDataAlloc.cpuAddress, &mBakeTerrainTexturePassData, sizeof(BakeTerrainTexturePassData));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mTerrainHeightMap, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainNormalMap, GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t threadGroupCountX = (mTerrainNormalMap->GetResourceFormat().GetTextureDesc().width + smThreadSizeInGroup - 1) / smThreadSizeInGroup;
			uint32_t threadGroupCountY = threadGroupCountX;
			commandBuffer.SetComputeRootSignature();
			commandBuffer.SetComputePipelineState(smBakeTerrainNormalMapSN);
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
		}

		// Second Pass Copy Terrain NormalMap to ReadbackBuffer
		{
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mTerrainNormalMap, GHL::EResourceState::CopySource);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainNormalMapReadbackBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			uint32_t subresourceCount = mTerrainNormalMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizesInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = mTerrainNormalMap->GetResourceFormat().D3DResourceDesc();
			mDevice->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
				placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

			for (uint32_t i = 0u; i < subresourceCount; i++) {
				auto& placedLayout = placedLayouts.at(i);

				// 将数据从显存复制到共享内存
				auto rbPlacedLayout = placedLayout;
				rbPlacedLayout.Footprint.RowPitch = (rbPlacedLayout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

				D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mTerrainNormalMap->D3DResource(), i);
				D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mTerrainNormalMapReadbackBuffer->D3DResource(), rbPlacedLayout);

				commandBuffer.D3DCommandList()->CopyTextureRegion(
					&dstLocation,
					0u, 0u, 0u,
					&srcLocation,
					nullptr
				);
			}
		}

		// Third Pass Bake Terrain AlbedoMap/NormalMap
		{
			const auto outputAlbedoMapDesc = mOutputAlbedoMap->GetResourceFormat().GetTextureDesc();

			mBakeTerrainTexturePassData.terrainHeightMapIndex = mTerrainHeightMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.terrainNormalMapIndex = mTerrainNormalMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.terrainSplatMapIndex = mTerrainSplatMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainTexturePassData.mvpMatrix;
			mBakeTerrainTexturePassData.tileOffset;
			mBakeTerrainTexturePassData.blendOffset;

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(BakeTerrainTexturePassData));
			memcpy(passDataAlloc.cpuAddress, &mBakeTerrainTexturePassData, sizeof(BakeTerrainTexturePassData));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mTerrainHeightMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainNormalMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainSplatMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mOutputAlbedoMap, GHL::EResourceState::RenderTarget);
			barrierBatch += commandBuffer.TransitionImmediately(mOutputNormalMap, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ mOutputAlbedoMap, mOutputNormalMap });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, outputAlbedoMapDesc.width, outputAlbedoMapDesc.height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, outputAlbedoMapDesc.width, outputAlbedoMapDesc.height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smBakeFarTerrainTextureSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, 1u, 0u, 0u, 0u);
		}
	}

	void TerrainTextureBaker::OnCompleted() {
		// 1. Terrain Normal Map
		{
			uint32_t startMipLevel = 0u;
			uint32_t subresourceCount = mTerrainNormalMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizesInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = mTerrainNormalMap->GetResourceFormat().D3DResourceDesc();
			mDevice->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
				placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

			std::vector<DirectX::Image> images(subresourceCount);
			for (uint32_t i = startMipLevel; i < subresourceCount; i++) {
				auto& image = images.at(i);
				auto& placedLayout = placedLayouts.at(i);

				image.width = placedLayout.Footprint.Width;
				image.height = placedLayout.Footprint.Height;
				image.format = placedLayout.Footprint.Format;
				image.rowPitch = placedLayout.Footprint.RowPitch;
				image.slicePitch = image.rowPitch * numRows.at(i);
				image.pixels = new uint8_t[image.slicePitch];

				// 将数据从共享内存复制到内存堆上
				memcpy(image.pixels, mTerrainNormalMapReadbackBuffer->Map() + placedLayout.Offset, image.slicePitch);
			}

			DirectX::TexMetadata metadata = GetTexMetadata(mTerrainNormalMap->GetResourceFormat().GetTextureDesc());
			DirectX::SaveToWICFile(images.data(), images.size(), WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), Tool::StrUtil::UTF8ToWString(mTerrainNormalMapPath).c_str());
		}
	}

	DirectX::TexMetadata TerrainTextureBaker::GetTexMetadata(const Renderer::TextureDesc& textureDesc) {
		DirectX::TexMetadata metadata{};
		metadata.width = textureDesc.width;
		metadata.height = textureDesc.height;
		metadata.depth = textureDesc.depth;
		metadata.arraySize = textureDesc.arraySize;
		metadata.mipLevels = textureDesc.mipLevals;
		metadata.miscFlags =
			HasAnyFlag(textureDesc.miscFlag, GHL::ETextureMiscFlag::CubeTexture) ?
			DirectX::TEX_MISC_TEXTURECUBE : 0u;
		metadata.miscFlags2;
		metadata.format = textureDesc.format;
		metadata.dimension =
			(textureDesc.dimension == GHL::ETextureDimension::Texture2D) ?
			DirectX::TEX_DIMENSION_TEXTURE2D : DirectX::TEX_DIMENSION_TEXTURE3D;

		return metadata;
	}

}