#include "MinMaxHeightMapGenerator.h"
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
        float patchTexScale = 0.0f; // MostDetailedPatch网格(1280 * 1280)相对于当前HeightMap的缩放
        uint32_t inMipMapIndex = 0u;
        uint32_t outMipMapIndex = 0u;
        float pad1 = 0.0f;
        float pad2 = 0.0f;
        float pad3 = 0.0f;
    };

	void MinMaxHeightMapGenerator::Initialize(const std::string& filename, RenderEngine* renderEngine) {
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
        request.Source.Memory.Size = baseImage.GetPixelsSize();
        request.Destination.Texture.Resource = mHeightMap->D3DResource();
        request.Destination.Texture.Region = GHL::Box{
            0u, _HeightMapDesc.width, 0u, _HeightMapDesc.height, 0u, _HeightMapDesc.depth
        }.D3DBox();
        request.Destination.Texture.SubresourceIndex = 0u;

        request.UncompressedSize = baseImage.GetPixelsSize();

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

        // 创建Shader1
        shaderManger->CreateComputeShader(
            "GenMinMaxHeightMap",
            [](ComputeStateProxy& proxy) {
                proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/MinMaxHeightMapGenerator.hlsl";
                proxy.csEntryPoint = "GenMinMaxHeightMap";
            }
        );

        // 创建Shader2
        shaderManger->CreateComputeShader(
            "GenMinMaxHeightMapMipMap",
            [](ComputeStateProxy& proxy) {
                proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/MinMaxHeightMapGenerator.hlsl";
                proxy.csEntryPoint = "GenMinMaxHeightMapMipMap";
            }
        );

        // 追踪资源状态
        resourceStateTracker->StartTracking(mHeightMap.get());
        resourceStateTracker->StartTracking(mMinMaxHeightMap.get());
	}

    void MinMaxHeightMapGenerator::Generate(CommandBuffer& commandBuffer, RenderContext& renderContext) {
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
                smMostDetailedPatchSize / smThreadSizeInGroup1,
                smMostDetailedPatchSize / smThreadSizeInGroup1,
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
                uint32_t threadGroupCountX = (smMostDetailedPatchSize / mis2) / smThreadSizeInGroup2;
                uint32_t threadGroupCountY = threadGroupCountX;

                commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
                commandBuffer.Dispatch(
                    threadGroupCountX,
                    threadGroupCountY,
                    1u
                );
            }
        }
    }

    void MinMaxHeightMapGenerator::OnCompleted() {

    }

    Renderer::TextureDesc MinMaxHeightMapGenerator::GetTextureDesc(const DirectX::TexMetadata& metadata) {
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

}