#include "Renderer/OceanPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/FixedTextureHelper.h"

#include <random>

namespace Renderer {

	void OceanPass::AddPass(RenderGraph& renderGraph) {
        auto& finalOutputDesc =
            renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

        renderGraph.AddPass("OceanBuilderPass",
            [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
                builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

                NewTextureProperties _GaussianRandomMapProperties{};
                _GaussianRandomMapProperties.width = smFFTSize;
                _GaussianRandomMapProperties.height = smFFTSize;
                _GaussianRandomMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("GaussianRandomMap", _GaussianRandomMapProperties);
                builder.WriteTexture("GaussianRandomMap");

                NewTextureProperties _HeightSpectrumMapProperties{};
                _HeightSpectrumMapProperties.width = smFFTSize;
                _HeightSpectrumMapProperties.height = smFFTSize;
                _HeightSpectrumMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("HeightSpectrumMap", _HeightSpectrumMapProperties);
                builder.WriteTexture("HeightSpectrumMap");

                NewTextureProperties _DisplaceXSpectrumMapProperties{};
                _DisplaceXSpectrumMapProperties.width = smFFTSize;
                _DisplaceXSpectrumMapProperties.height = smFFTSize;
                _DisplaceXSpectrumMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("DisplaceXSpectrumMap", _DisplaceXSpectrumMapProperties);
                builder.WriteTexture("DisplaceXSpectrumMap");

                NewTextureProperties _DisplaceZSpectrumMapProperties{};
                _DisplaceZSpectrumMapProperties.width = smFFTSize;
                _DisplaceZSpectrumMapProperties.height = smFFTSize;
                _DisplaceZSpectrumMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("DisplaceZSpectrumMap", _DisplaceZSpectrumMapProperties);
                builder.WriteTexture("DisplaceZSpectrumMap");

                NewTextureProperties _DisplaceMapProperties{};
                _DisplaceMapProperties.width = smFFTSize;
                _DisplaceMapProperties.height = smFFTSize;
                _DisplaceMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("DisplaceMap", _DisplaceMapProperties);
                builder.WriteTexture("DisplaceMap");

                NewTextureProperties _TempOutputMapProperties{};
                _TempOutputMapProperties.width = smFFTSize;
                _TempOutputMapProperties.height = smFFTSize;
                _TempOutputMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("TempOutputMap", _TempOutputMapProperties);
                builder.WriteTexture("TempOutputMap");

                NewTextureProperties _NormalMapProperties{};
                _NormalMapProperties.width = smFFTSize;
                _NormalMapProperties.height = smFFTSize;
                _NormalMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("NormalMap", _NormalMapProperties);
                builder.WriteTexture("NormalMap");

                NewTextureProperties _BubblesMapProperties{};
                _BubblesMapProperties.width = smFFTSize;
                _BubblesMapProperties.height = smFFTSize;
                _BubblesMapProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                builder.DeclareTexture("BubblesMap", _BubblesMapProperties);
                builder.WriteTexture("BubblesMap");

                shaderManger.CreateComputeShader("ComputeGaussianRandom",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "ComputeGaussianRandom";
                    });

                shaderManger.CreateComputeShader("CreateHeightSpectrum",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "CreateHeightSpectrum";
                    });

                shaderManger.CreateComputeShader("CreateDisplaceSpectrum",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "CreateDisplaceSpectrum";
                    });

                shaderManger.CreateComputeShader("FFTHorizontal",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "FFTHorizontal";
                    });

                shaderManger.CreateComputeShader("FFTHorizontalEnd",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "FFTHorizontalEnd";
                    });

                shaderManger.CreateComputeShader("FFTVertical",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "FFTVertical";
                    });

                shaderManger.CreateComputeShader("FFTVerticalEnd",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "FFTVerticalEnd";
                    });

                shaderManger.CreateComputeShader("GenerateDisplaceMap",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "GenerateDisplaceMap";
                    });

                shaderManger.CreateComputeShader("GenerateNormalBubblesMap",
                    [](ComputeStateProxy& proxy) {
                        proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanBuilder.hlsl";
                        proxy.csEntryPoint = "GenerateNormalBubblesMap";
                    });
            },
            [=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
                auto* dynamicAllocator = renderContext.dynamicAllocator;
                auto* resourceStorage = renderContext.resourceStorage;
                auto* commandSignatureManger = renderContext.commandSignatureManger;

                auto* gaussianRandomMap = resourceStorage->GetResourceByName("GaussianRandomMap")->GetTexture();
                auto* heightSpectrumMap = resourceStorage->GetResourceByName("HeightSpectrumMap")->GetTexture();
                auto* displaceXSpectrumMap = resourceStorage->GetResourceByName("DisplaceXSpectrumMap")->GetTexture();
                auto* displaceZSpectrumMap = resourceStorage->GetResourceByName("DisplaceZSpectrumMapIndex")->GetTexture();
                auto* displaceMap = resourceStorage->GetResourceByName("DisplaceMap")->GetTexture();
                auto* tempOutputMap = resourceStorage->GetResourceByName("TempOutputMap")->GetTexture();
                auto* normalMap = resourceStorage->GetResourceByName("NormalMap")->GetTexture();
                auto* bubblesMap = resourceStorage->GetResourceByName("BubblesMap")->GetTexture();

                oceanBuilderData.gaussianRandomMapIndex = gaussianRandomMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.heightSpectrumMapIndex = heightSpectrumMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.displaceXSpectrumMapIndex = displaceXSpectrumMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.displaceZSpectrumMapIndex = displaceZSpectrumMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.displaceMapIndex = displaceMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.tempOutputMapIndex = tempOutputMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.normalMapIndex = normalMap->GetUADescriptor()->GetHeapIndex();
                oceanBuilderData.bubblesMapIndex = bubblesMap->GetUADescriptor()->GetHeapIndex();
                
                oceanBuilderData.N = smFFTSize;
                oceanBuilderData.OceanLength = smMeshLength;
                
                Math::Vector2 wind = Math::Vector2(oceanBuilderData.WindAndSeed.x, oceanBuilderData.WindAndSeed.y).Normalize();
                oceanBuilderData.WindAndSeed.x = wind.x;
                oceanBuilderData.WindAndSeed.y = wind.y;

                std::random_device rd;
                std::default_random_engine eng(rd());
                std::uniform_int_distribution<float> distr(1.0f, 10.0f);
                oceanBuilderData.WindAndSeed.z = distr(eng);
                oceanBuilderData.WindAndSeed.w = distr(eng);

                auto passDataAlloc = dynamicAllocator->Allocate(sizeof(OceanBuilderData));
                memcpy(passDataAlloc.cpuAddress, &oceanBuilderData, sizeof(OceanBuilderData));

                commandBuffer.SetComputeRootSignature();
                commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
                commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
                commandBuffer.SetComputeRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);

                // 生成高斯随机数
                commandBuffer.SetComputePipelineState("ComputeGaussianRandom");
                commandBuffer.Dispatch(smFFTSize / smThreadSizeInGroup, smFFTSize / smThreadSizeInGroup, 1u);

                // 生成高度频谱
                commandBuffer.SetComputePipelineState("CreateHeightSpectrum");
                commandBuffer.Dispatch(smFFTSize / smThreadSizeInGroup, smFFTSize / smThreadSizeInGroup, 1u);

                // 生成偏移频谱
                commandBuffer.SetComputePipelineState("CreateDisplaceSpectrum");
                commandBuffer.Dispatch(smFFTSize / smThreadSizeInGroup, smFFTSize / smThreadSizeInGroup, 1u);

                // 进行横向FFT
                for (int m = 1; m <= smFFTPow; m++) {
                    int ns = (int)pow(2, m - 1);

                    // 最后一次进行特殊处理
                    if (m != smFFTPow) {
                        ComputeFFT(kernelFFTHorizontal, ref HeightSpectrumRT);
                        ComputeFFT(kernelFFTHorizontal, ref DisplaceXSpectrumRT);
                        ComputeFFT(kernelFFTHorizontal, ref DisplaceZSpectrumRT);
                    }
                    else {
                        ComputeFFT(kernelFFTHorizontalEnd, ref HeightSpectrumRT);
                        ComputeFFT(kernelFFTHorizontalEnd, ref DisplaceXSpectrumRT);
                        ComputeFFT(kernelFFTHorizontalEnd, ref DisplaceZSpectrumRT);
                    }
                }
                // 进行纵向FFT
                for (int m = 1; m <= smFFTPow; m++) {
                    int ns = (int)pow(2, m - 1);

                    // 最后一次进行特殊处理
                    if (m != smFFTPow) {
                        ComputeFFT(kernelFFTVertical, ref HeightSpectrumRT);
                        ComputeFFT(kernelFFTVertical, ref DisplaceXSpectrumRT);
                        ComputeFFT(kernelFFTVertical, ref DisplaceZSpectrumRT);
                    }
                    else {
                        ComputeFFT(kernelFFTVerticalEnd, ref HeightSpectrumRT);
                        ComputeFFT(kernelFFTVerticalEnd, ref DisplaceXSpectrumRT);
                        ComputeFFT(kernelFFTVerticalEnd, ref DisplaceZSpectrumRT);
                    }
                }

                // 计算纹理偏移
                commandBuffer.SetComputePipelineState("GenerateDisplaceMap");
                commandBuffer.Dispatch(smFFTSize / smThreadSizeInGroup, smFFTSize / smThreadSizeInGroup, 1u);

                // 生成法线和泡沫纹理
                commandBuffer.SetComputePipelineState("GenerateNormalBubblesMap");
                commandBuffer.Dispatch(smFFTSize / smThreadSizeInGroup, smFFTSize / smThreadSizeInGroup, 1u);
            });


		renderGraph.AddPass(
			"OceanRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
                builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

                builder.ReadTexture("TransmittanceLut", ShaderAccessFlag::PixelShader);
                builder.ReadTexture("SkyViewLut", ShaderAccessFlag::PixelShader);

                builder.WriteRenderTarget("ShadingResult");
                builder.WriteRenderTarget("ScreenVelocity");
                builder.WriteDepthStencil("DepthStencil");

                shaderManger.CreateGraphicsShader("OceanRenderer",
                    [](GraphicsStateProxy& proxy) {
                        proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Ocean/OceanRenderer.hlsl";
                        proxy.psFilepath = proxy.vsFilepath;
                        proxy.hsFilepath = proxy.vsFilepath;
                        proxy.dsFilepath = proxy.vsFilepath;

                        proxy.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
                        // proxy.rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

                        proxy.depthStencilDesc.DepthEnable = true;
                        proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
                        proxy.renderTargetFormatArray = {
                            DXGI_FORMAT_R16G16B16A16_FLOAT,
                            DXGI_FORMAT_R16G16_FLOAT,
                        };
                    });
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
                auto* dynamicAllocator = renderContext.dynamicAllocator;
                auto* resourceStorage = renderContext.resourceStorage;

                auto* shadingResult = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
                auto* screenVelocity = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
                auto* depthStencil = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();
                auto* transmittanceLut = resourceStorage->GetResourceByName("TransmittanceLut")->GetTexture();
                auto* skyViewLut = resourceStorage->GetResourceByName("SkyViewLut")->GetTexture();

                oceanPassData.waterParameter.waterNormalMap1Index = waterNormalMap1->GetSRDescriptor()->GetHeapIndex();
                oceanPassData.waterParameter.waterNormalMap2Index = waterNormalMap2->GetSRDescriptor()->GetHeapIndex();
                oceanPassData.waterParameter.waterFoamMapIndex = waterFoamMap->GetSRDescriptor()->GetHeapIndex();
                oceanPassData.waterParameter.waterNoiseMapIndex = waterNoiseMap->GetSRDescriptor()->GetHeapIndex();
                oceanPassData.transmittanceLutIndex = transmittanceLut->GetSRDescriptor()->GetHeapIndex();
                oceanPassData.skyViewLutIndex = skyViewLut->GetSRDescriptor()->GetHeapIndex();

                auto passDataAlloc = dynamicAllocator->Allocate(sizeof(OceanPassData));
                memcpy(passDataAlloc.cpuAddress, &oceanPassData, sizeof(OceanPassData));

                uint16_t width = static_cast<uint16_t>(finalOutputDesc.width);
                uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);
                commandBuffer.SetRenderTargets(
                    {
                        shadingResult,
                        screenVelocity,
                    },
                    depthStencil);
                commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
                commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
                commandBuffer.SetGraphicsRootSignature();
                commandBuffer.SetGraphicsPipelineState("OceanRenderer");
                commandBuffer.SetGraphicsRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
                commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
                commandBuffer.SetGraphicsRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
                commandBuffer.SetVertexBuffer(0u, gridMesh->GetVertexBuffer());
                commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
                commandBuffer.DrawInstanced(gridMesh->GetVertexCount(), 1u, 0u, 0u);
			}
			);
	}

	void OceanPass::InitializePass(RenderEngine* renderEngine) {
        auto* device = renderEngine->mDevice.get();
        auto* resourceAllocator = renderEngine->mResourceAllocator.get();
        auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
        auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
        auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
        auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();
        auto* resourceStorage = renderEngine->mPipelineResourceStorage;

        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            vertices.resize(smMeshSize * smMeshSize);
            indices.resize((smMeshSize - 1) * (smMeshSize - 1) * 6);

            int inx = 0;
            for (int i = 0; i < smMeshSize; i++) {
                for (int j = 0; j < smMeshSize; j++) {
                    int index = i * smMeshSize + j;

                    vertices[index].position = Math::Vector3{
                        (j - smMeshSize / 2.0f)* smMeshLength / smMeshSize,
                        0, 
                        (i - smMeshSize / 2.0f)* smMeshLength / smMeshSize
                    };
                    vertices[index].uv = Math::Vector2{
                        j / (smMeshSize - 1.0f), 
                        i / (smMeshSize - 1.0f)
                    };

                    if (i != smMeshSize - 1 && j != smMeshSize - 1) {
                        indices[inx++] = index;
                        indices[inx++] = index + smMeshSize;
                        indices[inx++] = index + smMeshSize + 1;

                        indices[inx++] = index;
                        indices[inx++] = index + smMeshSize + 1;
                        indices[inx++] = index + 1;
                    }
                }
            }

            Renderer::BufferDesc vbDesc{};
            vbDesc.stride = sizeof(Renderer::Vertex);
            vbDesc.size = vbDesc.stride * vertices.size();
            vbDesc.usage = GHL::EResourceUsage::Default;

            Renderer::BufferDesc ibDesc{};
            ibDesc.stride = sizeof(uint32_t);
            ibDesc.size = ibDesc.stride * indices.size();
            ibDesc.usage = GHL::EResourceUsage::Default;

            gridMesh = std::make_unique<Renderer::Mesh>(
                device,
                ResourceFormat{ device, vbDesc },
                ResourceFormat{ device, ibDesc },
                nullptr,
                nullptr);

            gridMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
        }

	}

}