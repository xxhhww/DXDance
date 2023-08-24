#include "Renderer/OceanPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/FixedTextureHelper.h"

namespace Renderer {

	void OceanPass::AddPass(RenderGraph& renderGraph) {
        auto& finalOutputDesc =
            renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

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
            uint32_t xTiles = 50u;	// 网格长度
            uint32_t zTiles = 50u;
            float texTileX = 5.0f;	// 纹理平铺
            float texTileZ = 5.0f;

            uint32_t vertexCount = xTiles * zTiles * 6u;

            //MeshVertex is just 2 float4's, one for position, one for texcoords
            std::vector<Vertex> vertices;
            vertices.resize(vertexCount);

            float oneOverXTiles = 1.0f / (float)xTiles;
            float oneOverZTiles = 1.0f / (float)zTiles;

            for (uint32_t x = 0; x < xTiles; x++) {
                for (uint32_t z = 0; z < zTiles; z++) {
                    uint32_t tileIndex = (x * zTiles + z) * 6;
                    float xBeginTile = (oneOverXTiles * (float)x) * texTileX;
                    float xEndTile = (oneOverXTiles * (float)(x + 1)) * texTileX;
                    float zBeginTile = (oneOverZTiles * (float)z) * texTileZ;
                    float zEndTile = (oneOverZTiles * (float)(z + 1)) * texTileZ;

                    float xBegin = (oneOverXTiles * (float)x);
                    float xEnd = (oneOverXTiles * (float)(x + 1));
                    float zBegin = (oneOverZTiles * (float)z);
                    float zEnd = (oneOverZTiles * (float)(z + 1));

                    vertices[tileIndex].position = Math::Vector3((float)x, 0.0f, (float)z);
                    vertices[tileIndex].color = Math::Vector4(xBeginTile, zBeginTile, xBegin, zBegin);
                    tileIndex++;

                    vertices[tileIndex].position = Math::Vector3((float)x, 0.0f, (float)z + 1.0f);
                    vertices[tileIndex].color = Math::Vector4(xBeginTile, zEndTile, xBegin, zEnd);
                    tileIndex++;

                    vertices[tileIndex].position = Math::Vector3((float)x + 1.0f, 0.0f, (float)z);
                    vertices[tileIndex].color = Math::Vector4(xEndTile, zBeginTile, xEnd, zBegin);
                    tileIndex++;

                    vertices[tileIndex].position = Math::Vector3((float)x, 0.0f, (float)z + 1.0f);
                    vertices[tileIndex].color = Math::Vector4(xBeginTile, zEndTile, xBegin, zEnd);
                    tileIndex++;

                    vertices[tileIndex].position = Math::Vector3((float)x + 1.0f, 0.0f, (float)z + 1.0f);
                    vertices[tileIndex].color = Math::Vector4(xEndTile, zEndTile, xEnd, zEnd);
                    tileIndex++;

                    vertices[tileIndex].position = Math::Vector3((float)x + 1.0f, 0.0f, (float)z);
                    vertices[tileIndex].color = Math::Vector4(xEndTile, zBeginTile, xEnd, zBegin);
                    tileIndex++;
                }
            }

            Renderer::BufferDesc vbDesc{};
            vbDesc.stride = sizeof(Renderer::Vertex);
            vbDesc.size = vbDesc.stride * vertices.size();
            vbDesc.usage = GHL::EResourceUsage::Default;

            gridMesh = std::make_unique<Renderer::Mesh>(
                device,
                ResourceFormat{ device, vbDesc },
                nullptr,
                nullptr);

            gridMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices);
        }

        {
            // 加载WaterNormalMap1
            waterNormalMap1 = FixedTextureHelper::LoadFromFile(
                device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
                "E:/MyProject/DXDance/Resources/Textures/Ocean/Water_Normal_1.dds");

            // 加载WaterNormalMap2
            waterNormalMap2 = FixedTextureHelper::LoadFromFile(
                device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
                "E:/MyProject/DXDance/Resources/Textures/Ocean/Water_Normal_2.dds"
            );
            
            // 加载WaterNoiseMap
            waterFoamMap = FixedTextureHelper::LoadFromFile(
                device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
                "E:/MyProject/DXDance/Resources/Textures/Ocean/Water_Foam.dds"
            );

            // 加载WaterNoiseMap
            waterNoiseMap = FixedTextureHelper::LoadFromFile(
                device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
                "E:/MyProject/DXDance/Resources/Textures/Ocean/Water_Noise.dds"
            );
        }

	}

}