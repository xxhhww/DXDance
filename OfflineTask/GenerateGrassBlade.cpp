#include <DirectStorage/dstorage.h>
#include <DirectXTex/DirectXTex.h>
#include <fstream>

#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"

#include "OfflineTask/GenerateGrassBlade.h"
#include "Math/Vector.h"
#include "GHL/Box.h"

namespace OfflineTask {

	void GenerateGrassBlade::Initialize(const std::string& heightMapFilepath, const std::string& grassLayerFilepath, Renderer::RenderEngine* renderEngine) {
		FillClumpParameters();

		auto* device = renderEngine->mDevice.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();

		static bool coInitialized = false;
		if (!coInitialized) {
			CoInitialize(nullptr);
		}

		{
			// 创建HeightMap
			DirectX::ScratchImage heightMapImage;
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(heightMapFilepath).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				heightMapImage
			));

			Renderer::TextureDesc _HeightMapDesc = GetTextureDesc(heightMapImage.GetMetadata());
			_HeightMapDesc.initialState = GHL::EResourceState::CopyDestination;
			_HeightMapDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;

			mTerrainHeightMap = std::make_unique<Renderer::Texture>(
				device,
				Renderer::ResourceFormat{ device, _HeightMapDesc },
				descriptorAllocator,
				nullptr
				);

			DSTORAGE_REQUEST request = {};
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
			request.Source.Memory.Source = heightMapImage.GetPixels();
			request.Source.Memory.Size = mTerrainHeightMap->GetResourceFormat().GetSizeInBytes();
			request.Destination.Texture.Resource = mTerrainHeightMap->D3DResource();
			request.Destination.Texture.Region = GHL::Box{
				0u, _HeightMapDesc.width, 0u, _HeightMapDesc.height, 0u, _HeightMapDesc.depth
			}.D3DBox();
			request.Destination.Texture.SubresourceIndex = 0u;
			request.UncompressedSize = mTerrainHeightMap->GetResourceFormat().GetSizeInBytes();
			copyDsQueue->EnqueueRequest(&request);
			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();
			heightMapImage.Release();
		}

		{
			// 创建GrassLayer
			DirectX::ScratchImage grassLayerMapImage;
			HRASSERT(DirectX::LoadFromWICFile(
				Tool::StrUtil::UTF8ToWString(grassLayerFilepath).c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				grassLayerMapImage
			));

			Renderer::TextureDesc _GrassLayerMapDesc = GetTextureDesc(grassLayerMapImage.GetMetadata());
			_GrassLayerMapDesc.initialState = GHL::EResourceState::CopyDestination;
			_GrassLayerMapDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;

			mTerrainGrassLayerMap = std::make_unique<Renderer::Texture>(
				device,
				Renderer::ResourceFormat{ device, _GrassLayerMapDesc },
				descriptorAllocator,
				nullptr
				);

			DSTORAGE_REQUEST request = {};
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
			request.Source.Memory.Source = grassLayerMapImage.GetPixels();
			request.Source.Memory.Size = mTerrainGrassLayerMap->GetResourceFormat().GetSizeInBytes();
			request.Destination.Texture.Resource = mTerrainGrassLayerMap->D3DResource();
			request.Destination.Texture.Region = GHL::Box{
				0u, _GrassLayerMapDesc.width, 0u, _GrassLayerMapDesc.height, 0u, _GrassLayerMapDesc.depth
			}.D3DBox();
			request.Destination.Texture.SubresourceIndex = 0u;
			request.UncompressedSize = mTerrainGrassLayerMap->GetResourceFormat().GetSizeInBytes();
			copyDsQueue->EnqueueRequest(&request);
			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();
			grassLayerMapImage.Release();
		}

		// 创建ClumpParametersBuffer
		Renderer::BufferDesc _ClumpParametersBufferDesc{};
		_ClumpParametersBufferDesc.size = sizeof(ClumpParameter) * mClumpParameters.size();
		_ClumpParametersBufferDesc.stride = 0u;
		_ClumpParametersBufferDesc.usage = GHL::EResourceUsage::Default;
		_ClumpParametersBufferDesc.initialState = GHL::EResourceState::CopyDestination;
		_ClumpParametersBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
		mClumpParametersBuffer = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, _ClumpParametersBufferDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建ClumpMap
		Renderer::TextureDesc _ClumpMapDesc{};
		_ClumpMapDesc.width = smClumpMapSize;
		_ClumpMapDesc.height = smClumpMapSize;
		_ClumpMapDesc.mipLevals = 0u;
		_ClumpMapDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		_ClumpMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
		_ClumpMapDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::CopySource;
		mClumpMap = std::make_unique<Renderer::Texture>(
			device,
			Renderer::ResourceFormat{ device, _ClumpMapDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建ClumpMapReadback
		Renderer::BufferDesc _ClumpMapReadbackDesc{};
		_ClumpMapReadbackDesc.size = Math::AlignUp(
			mClumpMap->GetResourceFormat().GetSizeInBytes(),
			D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
		);
		_ClumpMapReadbackDesc.stride = 0u;
		_ClumpMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
		_ClumpMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
		_ClumpMapReadbackDesc.expectedState = GHL::EResourceState::CopyDestination;
		mClumpMapReadback = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, _ClumpMapReadbackDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建GrassBladesBuffer
		Renderer::BufferDesc _GrassBladeBufferDesc{};
		_GrassBladeBufferDesc.size = sizeof(BakedGrassBlade) * smGrassResolution * smGrassResolution;
		_GrassBladeBufferDesc.stride = 0u;
		_GrassBladeBufferDesc.usage = GHL::EResourceUsage::Default;
		_GrassBladeBufferDesc.initialState = GHL::EResourceState::UnorderedAccess;
		_GrassBladeBufferDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::CopySource;
		mGrassBladesBuffer = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, _GrassBladeBufferDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建GrassBladesBufferReadback
		Renderer::BufferDesc _GrassBladeBufferReadbackDesc{};
		_GrassBladeBufferReadbackDesc.size = Math::AlignUp(
			mGrassBladesBuffer->GetResourceFormat().GetSizeInBytes(),
			D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
		);
		_GrassBladeBufferReadbackDesc.stride = 0u;
		_GrassBladeBufferReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
		_GrassBladeBufferReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
		_GrassBladeBufferReadbackDesc.expectedState = GHL::EResourceState::CopyDestination;
		mGrassBladesBufferReadback = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, _GrassBladeBufferReadbackDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建GrassBladesCountBufferReadback
		Renderer::BufferDesc _GrassBladesCountBufferReadbackDesc{};
		_GrassBladesCountBufferReadbackDesc.size = Math::AlignUp(
			mGrassBladesBuffer->GetCounterBuffer()->GetResourceFormat().GetSizeInBytes(),
			D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
		);
		_GrassBladesCountBufferReadbackDesc.stride = 0u;
		_GrassBladesCountBufferReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
		_GrassBladesCountBufferReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
		_GrassBladesCountBufferReadbackDesc.expectedState = GHL::EResourceState::CopyDestination;
		mGrassBladesCountBufferReadback = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, _GrassBladesCountBufferReadbackDesc },
			descriptorAllocator,
			nullptr
			);

		// 创建Shader1
		shaderManger->CreateComputeShader(
			"ClumpMapGenerator",
			[](Renderer::ComputeStateProxy& proxy) {
				proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/ClumpMapGenerator.hlsl";
			}
		);

		// 创建Shader2
		shaderManger->CreateComputeShader(
			"GrassBladeBaking",
			[](Renderer::ComputeStateProxy& proxy) {
				proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralGrass/GrassBladeBaking.hlsl";
			}
		);

		// 追踪资源状态
		resourceStateTracker->StartTracking(mTerrainHeightMap.get());
		resourceStateTracker->StartTracking(mTerrainGrassLayerMap.get());
		resourceStateTracker->StartTracking(mClumpParametersBuffer.get());
		resourceStateTracker->StartTracking(mClumpMap.get());
		resourceStateTracker->StartTracking(mClumpMapReadback.get());
		resourceStateTracker->StartTracking(mGrassBladesBuffer.get());
		resourceStateTracker->StartTracking(mGrassBladesBufferReadback.get());
	}

	void GenerateGrassBlade::Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext) {
		auto* dynamicAllocator = renderContext.dynamicAllocator;
		commandBuffer.SetComputeRootSignature();

		// First Pass Upload ClumpParameters To GPUBuffer
		{
			if (smFirstFrame) {
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(mClumpParametersBuffer.get(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(mClumpParametersBuffer.get(), 0u, mClumpParameters.data(), sizeof(ClumpParameter) * mClumpParameters.size());
			}
		}

		// Second Pass Generate ClumpMap
		{
			if (smFirstFrame) {
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(mClumpParametersBuffer.get(), GHL::EResourceState::NonPixelShaderAccess);
				barrierBatch += commandBuffer.TransitionImmediately(mClumpMap.get(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				mGenerateClumpMapPassData.clumpMapSize = Math::Vector2{ smClumpMapSize, smClumpMapSize };
				mGenerateClumpMapPassData.numClumps = mClumpParameters.size();
				mGenerateClumpMapPassData.clumpMapIndex = mClumpMap->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GenerateClumpMapPassData));
				memcpy(passDataAlloc.cpuAddress, &mGenerateClumpMapPassData, sizeof(GenerateClumpMapPassData));

				uint32_t threadGroupCountX = (smClumpMapSize + smThreadSizeInGroup - 1) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = threadGroupCountX;

				commandBuffer.SetComputePipelineState("ClumpMapGenerator");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			}
		}

		// Third Pass Generate GrassBladeBuffer
		{
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainHeightMap.get(), GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mClumpMap.get(), GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mClumpParametersBuffer.get(), GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mTerrainGrassLayerMap.get(), GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mGrassBladesBuffer.get(), GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mGrassBladesBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.ClearCounterBuffer(mGrassBladesBuffer.get(), 0u);
			barrierBatch = commandBuffer.TransitionImmediately(mGrassBladesBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			// 计算当前正在处理的TerrainTileRect
			Math::Vector2 ltOriginPos = Math::Vector2{ currColIndex * smGrassClusterMeterSize - smWorldMeterSize / 2.0f, -currRowIndex * smGrassClusterMeterSize + smWorldMeterSize / 2.0f };
			Math::Vector2 centerPos = Math::Vector2{ ltOriginPos.x + smGrassClusterMeterSize / 2.0f, ltOriginPos.y - smGrassClusterMeterSize / 2.0f };
			Math::Vector2 lbOriginPos = Math::Vector2{ centerPos.x - smGrassClusterMeterSize / 2.0f, centerPos.y - smGrassClusterMeterSize / 2.0f };
			mGenerateGrassBladePassData.terrainTileRect = Math::Vector4{ lbOriginPos.x, lbOriginPos.y, smGrassClusterMeterSize, smGrassClusterMeterSize };
			mGenerateGrassBladePassData.terrainWorldMeterSize = Math::Vector2{ smWorldMeterSize, smWorldMeterSize };
			mGenerateGrassBladePassData.grassResolution = smGrassResolution;
			mGenerateGrassBladePassData.terrainHeightMapIndex = mTerrainHeightMap->GetSRDescriptor()->GetHeapIndex();
			mGenerateGrassBladePassData.clumpMapIndex = mClumpMap->GetSRDescriptor()->GetHeapIndex();
			mGenerateGrassBladePassData.clumpParameterBufferIndex = mClumpParametersBuffer->GetSRDescriptor()->GetHeapIndex();
			mGenerateGrassBladePassData.grassLayerMapIndex = mTerrainGrassLayerMap->GetSRDescriptor()->GetHeapIndex();
			mGenerateGrassBladePassData.grassBladeBufferIndex = mGrassBladesBuffer->GetUADescriptor()->GetHeapIndex();

			auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GenerateGrassBladePassData));
			memcpy(passDataAlloc.cpuAddress, &mGenerateGrassBladePassData, sizeof(GenerateGrassBladePassData));

			uint32_t threadGroupCountX = (smGrassResolution + smThreadSizeInGroup - 1) / smThreadSizeInGroup;
			uint32_t threadGroupCountY = threadGroupCountX;

			commandBuffer.SetComputePipelineState("GrassBladeBaking");
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
		}

		// Fourth Pass: Copy GrassBladeBuffer to SharedMemory
		{
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += commandBuffer.TransitionImmediately(mGrassBladesBuffer.get(), GHL::EResourceState::CopySource);
			barrierBatch += commandBuffer.TransitionImmediately(mGrassBladesBufferReadback.get(), GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			size_t sizeInBytes = mGrassBladesBuffer->GetResourceFormat().GetSizeInBytes();
			commandBuffer.CopyBufferRegion(mGrassBladesBufferReadback.get(), 0u, mGrassBladesBuffer.get(), 0u, sizeInBytes);

			sizeInBytes = mGrassBladesBuffer->GetCounterBuffer()->GetResourceFormat().GetSizeInBytes();
			commandBuffer.CopyBufferRegion(mGrassBladesCountBufferReadback.get(), 0u, mGrassBladesBuffer.get(), 0, sizeInBytes);
		}

		// 更新参数
		{
			currColIndex ++;
			if (currColIndex % (uint32_t)smGrassClusterCountPerAxis == 0) {
				currColIndex = 0u;
				currRowIndex++;

				if (currRowIndex % (uint32_t)smGrassClusterCountPerAxis == 0) {
					ASSERT_FORMAT(false, "Mission Completed");
				}
			}
		}

		smFirstFrame = false;
	}

	// 最终保存到磁盘中的GrassBlade结构
	struct FinalGrassBladeStruct {
	public:
		Math::Vector3 position;
		float    height;
		float    width;

		uint8_t angle;	// 朝向(角度) 将0 - 360 转换到 0 - 255
		uint8_t tilt;	// 描述草叶的倾斜状态(tilt应该是一个0-1的值，可转换为0-255)
		uint8_t bend;	// 控制草叶的弯曲(其实就是控制贝塞尔样条曲线，是一个0-1的值，可转换为0-255)
		// float sideCurve;	// 控制草叶的边的弯曲
	};

	void GenerateGrassBlade::OnCompleted() {
		// 当前帧任务完成，将结果保存到磁盘中

		uint32_t* pCounterBufferReadbackData = reinterpret_cast<uint32_t*>(mGrassBladesCountBufferReadback->Map());
		uint32_t bladeCount = pCounterBufferReadbackData[0];

		if (bladeCount <= 0u) {
			return;
		}

		// 创建文件二进制输出流
		std::string outputFilename = "GrassBlade_" + std::to_string(currRowIndex) + "_" + std::to_string(currColIndex);
		std::ofstream outFileStream(outputFilename.c_str(), std::ios::app | std::ios::binary);

		BakedGrassBlade* pBakedGrassBladesData = reinterpret_cast<BakedGrassBlade*>(mGrassBladesBufferReadback->Map());
		for (uint32_t i = 0; i < bladeCount; i++) {
			BakedGrassBlade blade = pBakedGrassBladesData[i];
			FinalGrassBladeStruct finalBlade{};
			finalBlade.position = blade.position;
			finalBlade.height = blade.height;
			finalBlade.width = blade.width;
			float angle = std::atan2(blade.facing.y, blade.facing.x);
			angle = angle < 0.0f ? angle + 360.0f : angle;
			finalBlade.angle = (angle / 360.0f) * 255u;
			finalBlade.tilt = blade.tilt * 255u;
			finalBlade.bend = blade.bend * 255u;

			outFileStream.write((char*)&finalBlade.position.x, sizeof(float));
			outFileStream.write((char*)&finalBlade.position.y, sizeof(float));
			outFileStream.write((char*)&finalBlade.position.z, sizeof(float));
			outFileStream.write((char*)&finalBlade.height, sizeof(float));
			outFileStream.write((char*)&finalBlade.width, sizeof(float));
			outFileStream.write((char*)&finalBlade.angle, sizeof(uint8_t));
			outFileStream.write((char*)&finalBlade.tilt, sizeof(uint8_t));
			outFileStream.write((char*)&finalBlade.bend, sizeof(uint8_t));
		}
		outFileStream.close();
	}

	void GenerateGrassBlade::FillClumpParameters() {
		mClumpParameters.resize(3u);

		mClumpParameters[0];
		mClumpParameters[0].pullToCentre = 0;
		mClumpParameters[0].pointInSameDirection = 0;
		mClumpParameters[0].baseHeight = 1;
		mClumpParameters[0].heightRandom = 0.5;
		mClumpParameters[0].baseWidth = 0.02;
		mClumpParameters[0].widthRandom = 0.02;
		mClumpParameters[0].baseTilt = 0.8;
		mClumpParameters[0].tiltRandom = 0.5;
		mClumpParameters[0].baseBend = 0.1;
		mClumpParameters[0].bendRandom = 0.05;

		mClumpParameters[1];
		mClumpParameters[1].pullToCentre = 0;
		mClumpParameters[1].pointInSameDirection = 0;
		mClumpParameters[1].baseHeight = 1.2;
		mClumpParameters[1].heightRandom = 0.5;
		mClumpParameters[1].baseWidth = 0.02;
		mClumpParameters[1].widthRandom = 0.02;
		mClumpParameters[1].baseTilt = 0.9;
		mClumpParameters[1].tiltRandom = 0;
		mClumpParameters[1].baseBend = 0.1;
		mClumpParameters[1].bendRandom = 0.05;

		mClumpParameters[2];
		mClumpParameters[2].pullToCentre = 0;
		mClumpParameters[2].pointInSameDirection = 0;
		mClumpParameters[2].baseHeight = 1.1;
		mClumpParameters[2].heightRandom = 0.5;
		mClumpParameters[2].baseWidth = 0.02;
		mClumpParameters[2].widthRandom = 0.02;
		mClumpParameters[2].baseTilt = 0.8;
		mClumpParameters[2].tiltRandom = 0;
		mClumpParameters[2].baseBend = 0.1;
		mClumpParameters[2].bendRandom = 0.05;
	}

	Renderer::TextureDesc GenerateGrassBlade::GetTextureDesc(const DirectX::TexMetadata& metadata) {
		Renderer::TextureDesc desc{};
		desc.dimension =
			metadata.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D ?
			GHL::ETextureDimension::Texture2D : GHL::ETextureDimension::Texture3D;
		desc.width = metadata.width;
		desc.height = metadata.height;
		desc.depth = metadata.depth;
		desc.arraySize = metadata.arraySize;
		desc.mipLevals = metadata.mipLevels;
		desc.sampleCount = 1u;
		desc.format = metadata.format;
		desc.usage = GHL::EResourceUsage::Default;
		desc.miscFlag = metadata.IsCubemap() ?
			GHL::ETextureMiscFlag::CubeTexture : GHL::ETextureMiscFlag::None;
		desc.createdMethod = GHL::ECreatedMethod::Committed;

		return desc;
	}

	DirectX::TexMetadata GenerateGrassBlade::GetTexMetadata(const Renderer::TextureDesc& textureDesc) {
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