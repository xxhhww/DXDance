#include "TerrainOfflineTask.h"
#include <wincodec.h>

#include "Renderer/LinearBufferAllocator.h"

#include "GHL/Box.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

    struct GenMinMaxHeightMapPassData {
        uint32_t heightMapIndex = 0u;
        uint32_t minMaxHeightMapIndex = 0u;
        float patchTexScale = 0.0f; // MostDetailedPatch网格(640 * 640)相对于当前HeightMap的缩放
        uint32_t inMipMapIndex = 0u;
        uint32_t outMipMapIndex = 0u;
        uint32_t normalMapIndex = 0u;
        uint32_t heightMapWidth = 0u;
        uint32_t heightMapHeight = 0u;
        // Math::Vector2 worldMeterSize{ 5120u, 5120u };
        // Math::Vector2 worldMeterSize{ 10240u, 10240u };
        Math::Vector2 worldMeterSize{ 8192u, 8192u };
        float worldHeightScale{ 1325.0f };
        float pad1;
    };

	void TerrainOfflineTask::Initialize(const std::string& filename, RenderEngine* renderEngine) {
        auto* device = renderEngine->mDevice.get();
        auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
        auto* shaderManger = renderEngine->mShaderManger.get();
        auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();
        auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
        auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();

        mDevice = device;

        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }

        DirectX::ScratchImage baseImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(filename).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            baseImage
        ));

        Renderer::TextureDesc _HeightMapDesc = GetTextureDesc(baseImage.GetMetadata());
        _HeightMapDesc.initialState = GHL::EResourceState::CopyDestination;
        _HeightMapDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;

        mHeightMap = std::make_unique<Texture>(
            device,
            ResourceFormat{ device, _HeightMapDesc },
            descriptorAllocator,
            nullptr
            );

        // 上传数据到显存

        DSTORAGE_REQUEST request = {};

        request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
        request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;

        request.Source.Memory.Source = baseImage.GetPixels();
        request.Source.Memory.Size = mHeightMap->GetResourceFormat().GetSizeInBytes();
        request.Destination.Texture.Resource = mHeightMap->D3DResource();
        request.Destination.Texture.Region = GHL::Box{
            0u, _HeightMapDesc.width, 0u, _HeightMapDesc.height, 0u, _HeightMapDesc.depth
        }.D3DBox();
        request.Destination.Texture.SubresourceIndex = 0u;

        request.UncompressedSize = mHeightMap->GetResourceFormat().GetSizeInBytes();
        
        copyDsQueue->EnqueueRequest(&request);
        copyFence->IncrementExpectedValue();
        copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
        copyDsQueue->Submit();
        copyFence->Wait();
        
        // 数据已经上传到显存，内存中的数据可以释放
        baseImage.Release();

        // 创建MinMaxHeightMap
        Renderer::TextureDesc _MinMaxHeightMapDesc{};
        _MinMaxHeightMapDesc.width = smMostDetailedPatchSize;
        _MinMaxHeightMapDesc.height = smMostDetailedPatchSize;
        _MinMaxHeightMapDesc.mipLevals = 9u;
        _MinMaxHeightMapDesc.format = DXGI_FORMAT_R16G16B16A16_UNORM;
        _MinMaxHeightMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
        _MinMaxHeightMapDesc.expectedState = GHL::EResourceState::UnorderedAccess | GHL::EResourceState::NonPixelShaderAccess;

        mMinMaxHeightMap = std::make_unique<Texture>(
            device,
            ResourceFormat{ device, _MinMaxHeightMapDesc },
            descriptorAllocator,
            nullptr
            );

        // 创建NormalMap
        Renderer::TextureDesc _NormalMapDesc{};
        _NormalMapDesc.width = _HeightMapDesc.width;
        _NormalMapDesc.height = _HeightMapDesc.height;
        _NormalMapDesc.mipLevals = 1u;
        _NormalMapDesc.format = DXGI_FORMAT_R16G16B16A16_UNORM;
        _NormalMapDesc.initialState = GHL::EResourceState::UnorderedAccess;
        _NormalMapDesc.expectedState = _NormalMapDesc.initialState;

        mNormalMap = std::make_unique<Texture>(
            device,
            ResourceFormat{ device, _NormalMapDesc },
            descriptorAllocator,
            nullptr
            );

        // 创建共享内存映射
        Renderer::BufferDesc _MinMaxHeightMapReadbackDesc{};
        _MinMaxHeightMapReadbackDesc.size = Math::AlignUp(
            mMinMaxHeightMap->GetResourceFormat().GetSizeInBytes(), 
            D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
        );
        _MinMaxHeightMapReadbackDesc.stride = 0u;
        _MinMaxHeightMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
        _MinMaxHeightMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
        _MinMaxHeightMapReadbackDesc.expectedState = _MinMaxHeightMapReadbackDesc.initialState;
        mMinMaxHeightMapReadback = std::make_unique<Buffer>(
            device,
            ResourceFormat{ device, _MinMaxHeightMapReadbackDesc },
            descriptorAllocator,
            nullptr
            );

        Renderer::BufferDesc _NormalMapReadbackDesc{};
        _NormalMapReadbackDesc.size = Math::AlignUp(
            mNormalMap->GetResourceFormat().GetSizeInBytes(),
            D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
        );
        _NormalMapReadbackDesc.stride = 0u;
        _NormalMapReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
        _NormalMapReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
        _NormalMapReadbackDesc.expectedState = _NormalMapReadbackDesc.initialState;
        mNormalMapReadback = std::make_unique<Buffer>(
            device,
            ResourceFormat{ device, _NormalMapReadbackDesc },
            descriptorAllocator,
            nullptr
            );

        // 创建Shader1
        shaderManger->CreateComputeShader(
            "GenMinMaxHeightMap",
            [](ComputeStateProxy& proxy) {
                proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainOfflineTask.hlsl";
                proxy.csEntryPoint = "GenMinMaxHeightMap";
            }
        );

        // 创建Shader2
        shaderManger->CreateComputeShader(
            "GenMinMaxHeightMapMipMap",
            [](ComputeStateProxy& proxy) {
                proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainOfflineTask.hlsl";
                proxy.csEntryPoint = "GenMinMaxHeightMapMipMap";
            }
        );

        // 创建GenNormalMap Shader
        shaderManger->CreateComputeShader(
            "GenNormalMap",
            [](ComputeStateProxy& proxy) {
                proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainOfflineTask.hlsl";
                proxy.csEntryPoint = "GenNormalMap";
            }
        );

        // 追踪资源状态
        resourceStateTracker->StartTracking(mHeightMap.get());
        resourceStateTracker->StartTracking(mMinMaxHeightMap.get());
        resourceStateTracker->StartTracking(mNormalMap.get());
        resourceStateTracker->StartTracking(mMinMaxHeightMapReadback.get());
        resourceStateTracker->StartTracking(mNormalMapReadback.get());
	}

    void TerrainOfflineTask::Generate(CommandBuffer& commandBuffer, RenderContext& renderContext) {
        auto* dynamicAllocator = renderContext.dynamicAllocator;
        commandBuffer.SetComputeRootSignature();

        // First Pass: GenMinMaxHeightMap
        {
            GenMinMaxHeightMapPassData genMinMaxHeightMapPassData;
            genMinMaxHeightMapPassData.heightMapIndex = mHeightMap->GetSRDescriptor()->GetHeapIndex();
            genMinMaxHeightMapPassData.minMaxHeightMapIndex = mMinMaxHeightMap->GetUADescriptor()->GetHeapIndex();
            genMinMaxHeightMapPassData.patchTexScale =
                (float)mHeightMap->GetResourceFormat().GetTextureDesc().width / (float)mMinMaxHeightMap->GetResourceFormat().GetTextureDesc().width;

            auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GenMinMaxHeightMapPassData));
            memcpy(passDataAlloc.cpuAddress, &genMinMaxHeightMapPassData, sizeof(GenMinMaxHeightMapPassData));

            auto barrierBatch = GHL::ResourceBarrierBatch{};
            barrierBatch =  commandBuffer.TransitionImmediately(mHeightMap.get(), GHL::EResourceState::NonPixelShaderAccess);
            barrierBatch += commandBuffer.TransitionImmediately(mMinMaxHeightMap.get(), 0u, GHL::EResourceState::UnorderedAccess);
            commandBuffer.FlushResourceBarrier(barrierBatch);

            commandBuffer.SetComputePipelineState("GenMinMaxHeightMap");
            commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
            commandBuffer.Dispatch(
                (smMostDetailedPatchSize + smThreadSizeInGroup1 - 1) / smThreadSizeInGroup1,
                (smMostDetailedPatchSize + smThreadSizeInGroup1 - 1) / smThreadSizeInGroup1,
                1u
            );
        }

        // Second Pass: GenMinMaxHeightMapMipMap
        {
            commandBuffer.SetComputePipelineState("GenMinMaxHeightMapMipMap");
            auto barrierBatch = GHL::ResourceBarrierBatch{};
            GenMinMaxHeightMapPassData genMinMaxHeightMapPassData;
            uint32_t mipLevels = mMinMaxHeightMap->GetResourceFormat().GetTextureDesc().mipLevals;
            for (uint32_t i = 0u; i < mipLevels - 1u; i++) {
                
                TextureSubResourceDesc inSubResourceDesc{};
                inSubResourceDesc.firstMip = i;
                inSubResourceDesc.mipCount = 1u;
                TextureSubResourceDesc outSubresourceDesc{};
                outSubresourceDesc.firstMip = i + 1u;
                outSubresourceDesc.mipCount = 1u;

                genMinMaxHeightMapPassData.inMipMapIndex = mMinMaxHeightMap->GetSRDescriptor(inSubResourceDesc)->GetHeapIndex();
                genMinMaxHeightMapPassData.outMipMapIndex = mMinMaxHeightMap->GetUADescriptor(outSubresourceDesc)->GetHeapIndex();
                
                auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GenMinMaxHeightMapPassData));
                memcpy(passDataAlloc.cpuAddress, &genMinMaxHeightMapPassData, sizeof(GenMinMaxHeightMapPassData));

                barrierBatch =  commandBuffer.TransitionImmediately(mMinMaxHeightMap.get(), i, GHL::EResourceState::NonPixelShaderAccess);
                barrierBatch += commandBuffer.TransitionImmediately(mMinMaxHeightMap.get(), i + 1, GHL::EResourceState::UnorderedAccess);
                commandBuffer.FlushResourceBarrier(barrierBatch);

                uint32_t mis2 = pow(2, i + 1u);
                uint32_t threadGroupCountX = ((smMostDetailedPatchSize / mis2) + smThreadSizeInGroup2 - 1u) / smThreadSizeInGroup2;
                uint32_t threadGroupCountY = threadGroupCountX;

                commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
                commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY,1u);
            }
        }

        // Third Pass: GenNormalMap
        {
            GenMinMaxHeightMapPassData genMinMaxHeightMapPassData;
            genMinMaxHeightMapPassData.heightMapIndex = mHeightMap->GetSRDescriptor()->GetHeapIndex();
            genMinMaxHeightMapPassData.normalMapIndex = mNormalMap->GetUADescriptor()->GetHeapIndex();
            genMinMaxHeightMapPassData.heightMapWidth = mNormalMap->GetResourceFormat().GetTextureDesc().width;
            genMinMaxHeightMapPassData.heightMapHeight = mNormalMap->GetResourceFormat().GetTextureDesc().height;

            auto passDataAlloc = dynamicAllocator->Allocate(sizeof(GenMinMaxHeightMapPassData));
            memcpy(passDataAlloc.cpuAddress, &genMinMaxHeightMapPassData, sizeof(GenMinMaxHeightMapPassData));

            auto barrierBatch = GHL::ResourceBarrierBatch{};
            barrierBatch =  commandBuffer.TransitionImmediately(mHeightMap.get(), GHL::EResourceState::NonPixelShaderAccess);
            barrierBatch += commandBuffer.TransitionImmediately(mNormalMap.get(), GHL::EResourceState::UnorderedAccess);
            commandBuffer.FlushResourceBarrier(barrierBatch);

            uint32_t threadGroupCountX = mNormalMap->GetResourceFormat().GetTextureDesc().width / smThreadSizeInGroup3;
            uint32_t threadGroupCountY = threadGroupCountX;

            commandBuffer.SetComputePipelineState("GenNormalMap");
            commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
            commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
        }

        // Fourth Pass: Copy MinMaxHeightMap From VideoMemory To SharedMemory
        {
            auto barrierBatch = GHL::ResourceBarrierBatch{};
            barrierBatch = commandBuffer.TransitionImmediately(mMinMaxHeightMap.get(), GHL::EResourceState::CopySource);
            barrierBatch += commandBuffer.TransitionImmediately(mMinMaxHeightMapReadback.get(), GHL::EResourceState::CopyDestination);
            commandBuffer.FlushResourceBarrier(barrierBatch);

            uint32_t subresourceCount = mMinMaxHeightMap->GetResourceFormat().SubresourceCount();
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
            std::vector<uint32_t> numRows(subresourceCount);
            std::vector<uint64_t> rowSizesInBytes(subresourceCount);
            uint64_t requiredSize = 0u;
            auto d3dResDesc = mMinMaxHeightMap->GetResourceFormat().D3DResourceDesc();
            mDevice->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
                placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

            for (uint32_t i = 0u; i < subresourceCount; i++) {
                auto& placedLayout = placedLayouts.at(i);

                // 将数据从显存复制到共享内存
                auto rbPlacedLayout = placedLayout;
                rbPlacedLayout.Footprint.RowPitch = (rbPlacedLayout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

                D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mMinMaxHeightMap->D3DResource(), i);
                D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mMinMaxHeightMapReadback->D3DResource(), rbPlacedLayout);

                commandBuffer.D3DCommandList()->CopyTextureRegion(
                    &dstLocation,
                    0u, 0u, 0u,
                    &srcLocation,
                    nullptr
                );
            }
        }

        // Fifth Pass: Copy NormalMap From VideoMemory To SharedMemory
        {
            auto barrierBatch = GHL::ResourceBarrierBatch{};
            barrierBatch = commandBuffer.TransitionImmediately(mNormalMap.get(), GHL::EResourceState::CopySource);
            barrierBatch += commandBuffer.TransitionImmediately(mNormalMapReadback.get(), GHL::EResourceState::CopyDestination);
            commandBuffer.FlushResourceBarrier(barrierBatch);

            uint32_t subresourceCount = mNormalMap->GetResourceFormat().SubresourceCount();
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
            std::vector<uint32_t> numRows(subresourceCount);
            std::vector<uint64_t> rowSizesInBytes(subresourceCount);
            uint64_t requiredSize = 0u;
            auto d3dResDesc = mNormalMap->GetResourceFormat().D3DResourceDesc();
            mDevice->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, subresourceCount, 0u,
                placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

            for (uint32_t i = 0u; i < subresourceCount; i++) {
                auto& placedLayout = placedLayouts.at(i);

                // 将数据从显存复制到共享内存
                auto rbPlacedLayout = placedLayout;
                rbPlacedLayout.Footprint.RowPitch = (rbPlacedLayout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

                D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mNormalMap->D3DResource(), i);
                D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mNormalMapReadback->D3DResource(), rbPlacedLayout);

                commandBuffer.D3DCommandList()->CopyTextureRegion(
                    &dstLocation,
                    0u, 0u, 0u,
                    &srcLocation,
                    nullptr
                );
            }

        }
    }

    void TerrainOfflineTask::OnCompleted() {
        // 离线任务完成，将结果保存到磁盘中

        // 1. MinMaxHeightMap
        {
            uint32_t startMipLevel = 0u;
            uint32_t subresourceCount = mMinMaxHeightMap->GetResourceFormat().SubresourceCount();
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
            std::vector<uint32_t> numRows(subresourceCount);
            std::vector<uint64_t> rowSizesInBytes(subresourceCount);
            uint64_t requiredSize = 0u;
            auto d3dResDesc = mMinMaxHeightMap->GetResourceFormat().D3DResourceDesc();
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
                memcpy(image.pixels, mMinMaxHeightMapReadback->Map() + placedLayout.Offset, image.slicePitch);
            }

            DirectX::TexMetadata metadata = GetTexMetadata(mMinMaxHeightMap->GetResourceFormat().GetTextureDesc());
            DirectX::SaveToWICFile(images[0], WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), L"Test_1.png");
            // DirectX::SaveToDDSFile(images.data(), images.size(), metadata, DirectX::DDS_FLAGS_FORCE_DX10_EXT, L"MinMaxHeightMap_1.dds");
        }

        // 2. NormalMap
        {
            uint32_t startMipLevel = 0u;
            uint32_t subresourceCount = mNormalMap->GetResourceFormat().SubresourceCount();
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
            std::vector<uint32_t> numRows(subresourceCount);
            std::vector<uint64_t> rowSizesInBytes(subresourceCount);
            uint64_t requiredSize = 0u;
            auto d3dResDesc = mNormalMap->GetResourceFormat().D3DResourceDesc();
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
                memcpy(image.pixels, mNormalMapReadback->Map() + placedLayout.Offset, image.slicePitch);
            }

            DirectX::TexMetadata metadata = GetTexMetadata(mNormalMap->GetResourceFormat().GetTextureDesc());
            DirectX::SaveToWICFile(images.data(), images.size(), WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), L"NormalMap_1.png");
            // DirectX::SaveToDDSFile(images.data(), images.size(), metadata, DirectX::DDS_FLAGS_NONE, L"NormalMap_2.dds");
        }
    }

    Renderer::TextureDesc TerrainOfflineTask::GetTextureDesc(const DirectX::TexMetadata& metadata) {
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

    DirectX::TexMetadata TerrainOfflineTask::GetTexMetadata(const Renderer::TextureDesc& textureDesc) {
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