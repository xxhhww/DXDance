#include <DirectStorage/dstorage.h>
#include <fstream>

#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/Misc.h"

#include "OfflineTask/BakeTerrainAlbedo.h"
#include "Math/Vector.h"
#include "GHL/Box.h"

namespace OfflineTask {

	void BakeTerrainAlbedo::Initialize(
		const std::string& heightMapFilepath,
		const std::string& normalMapFilepath,
		const std::string& splatMapFilepath,
		const std::string& albedoOutputPath,
		const std::string& normalOutputPath,
		Renderer::RenderEngine* renderEngine) {

		auto* device = renderEngine->mDevice.get();
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

		// Load NormalMap From File
		{
			mTerrainNormalMap = Renderer::FixedTextureHelper::LoadFromFile(device, descriptorAllocator, resourceAllocator, dstorageQueue->GetDStorageQueue(), dstorageFence, normalMapFilepath);
			resourceStateTracker->StartTracking(mTerrainNormalMap);
			resourceStorage->ImportResource("TerrainNormalMap", mTerrainNormalMap);
		}

		// Load SplatMap From File
		{
			mTerrainSplatMap = Renderer::FixedTextureHelper::LoadFromFile(device, descriptorAllocator, resourceAllocator, dstorageQueue->GetDStorageQueue(), dstorageFence, splatMapFilepath);
			resourceStateTracker->StartTracking(mTerrainSplatMap);
			resourceStorage->ImportResource("TerrainSplatMap", mTerrainSplatMap);
		}

		// 创建AlbedoMap 与 ReadbackBuffer
		{
			Renderer::TextureDesc _OutputAlbedoDesc{};
			_OutputAlbedoDesc.width = smTerrainMeshVertexCountPerAxis;
			_OutputAlbedoDesc.height = smTerrainMeshVertexCountPerAxis;
			_OutputAlbedoDesc.mipLevals = 1u;
			_OutputAlbedoDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_OutputAlbedoDesc.initialState = GHL::EResourceState::RenderTarget;
			_OutputAlbedoDesc.expectedState = GHL::EResourceState::RenderTarget;
			mOutputAlbedoMap = resourceAllocator->Allocate(device, _OutputAlbedoDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputAlbedoMap);
			resourceStorage->ImportResource("OutputAlbedoMap", mOutputAlbedoMap);

			Renderer::BufferDesc _OutputAlbedoMapReadbackDesc{};
			_OutputAlbedoMapReadbackDesc.size = Math::AlignUp(mOutputAlbedoMap->GetResourceFormat().GetSizeInBytes(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			_OutputAlbedoMapReadbackDesc.stride = 0u;
			_OutputAlbedoMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_OutputAlbedoMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_OutputAlbedoMapReadbackDesc.expectedState = GHL::EResourceState::CopyDestination;
			mOutputAlbedoReadbackBuffer = resourceAllocator->Allocate(device, _OutputAlbedoMapReadbackDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputAlbedoReadbackBuffer);
			resourceStorage->ImportResource("OutputAlbedoReadbackBuffer", mOutputAlbedoReadbackBuffer);
		}

		// 创建NormalMap 与 ReadbackBuffer
		{
			Renderer::TextureDesc _OutputNormalDesc{};
			_OutputNormalDesc.width = smTerrainMeshVertexCountPerAxis;
			_OutputNormalDesc.height = smTerrainMeshVertexCountPerAxis;
			_OutputNormalDesc.mipLevals = 1u;
			_OutputNormalDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_OutputNormalDesc.initialState = GHL::EResourceState::RenderTarget;
			_OutputNormalDesc.expectedState = GHL::EResourceState::RenderTarget;
			mOutputNormalMap = resourceAllocator->Allocate(device, _OutputNormalDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputNormalMap);
			resourceStorage->ImportResource("OutputNormalMap", mOutputNormalMap);

			Renderer::BufferDesc _OutputNormalMapReadbackDesc{};
			_OutputNormalMapReadbackDesc.size = Math::AlignUp(mOutputNormalMap->GetResourceFormat().GetSizeInBytes(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			_OutputNormalMapReadbackDesc.stride = 0u;
			_OutputNormalMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_OutputNormalMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_OutputNormalMapReadbackDesc.expectedState = GHL::EResourceState::CopyDestination;
			mOutputNormalReadbackBuffer = resourceAllocator->Allocate(device, _OutputNormalMapReadbackDesc, descriptorAllocator, nullptr);
			resourceStateTracker->StartTracking(mOutputNormalReadbackBuffer);
			resourceStorage->ImportResource("OutputNormalReadbackBuffer", mOutputNormalReadbackBuffer);
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
			shaderManger->CreateGraphicsShader(smBakeTerrainTextureSN,
				[&](Renderer::GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/TerrainTextureBaker.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mOutputAlbedoMap->GetResourceFormat().GetTextureDesc().format,	// OutputAlbedoMap
						mOutputNormalMap->GetResourceFormat().GetTextureDesc().format	// OutputNormalMap
					};
				});
		}
	}

	void BakeTerrainAlbedo::Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext) {
		auto* dynamicAllocator = renderContext.dynamicAllocator;

		// First Pass Bake Terrain AlbedoMap/NormalMap
		{
			mBakeTerrainAlbedoPassData.terrainHeightMapIndex = mTerrainHeightMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainAlbedoPassData.terrainNormalMapIndex = mTerrainNormalMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainAlbedoPassData.terrainSplatMapIndex = mTerrainSplatMap->GetSRDescriptor()->GetHeapIndex();
			mBakeTerrainAlbedoPassData.tileOffset;
			mBakeTerrainAlbedoPassData.blendOffset;
			mBakeTerrainAlbedoPassData.mvpMatrix;

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(BakeTerrainAlbedoPassData));
			memcpy(passDataAlloc.cpuAddress, &mBakeTerrainAlbedoPassData, sizeof(BakeTerrainAlbedoPassData));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  commandBuffer.TransitionImmediately(mTerrainHeightMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainNormalMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainSplatMap, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mOutputAlbedoMap, GHL::EResourceState::RenderTarget);
			barrierBatch += commandBuffer.TransitionImmediately(mOutputNormalMap, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ mOutputAlbedoMap, mOutputNormalMap });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, (float)smTerrainMeshVertexCountPerAxis, (float)smTerrainMeshVertexCountPerAxis });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, (float)smTerrainMeshVertexCountPerAxis, (float)smTerrainMeshVertexCountPerAxis });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smBakeTerrainTextureSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, 1u, 0u, 0u, 0u);
		}
	}

	void BakeTerrainAlbedo::OnCompleted() {

	}

}