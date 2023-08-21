#include "Renderer/AtmospherePass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	void AtmospherePass::AddBuilderPass(RenderGraph& renderGraph) {
		renderGraph.AddPass("AtmosphereBuilderPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewTextureProperties _TransmittanceLutProperties{};
				_TransmittanceLutProperties.width = 256u;
				_TransmittanceLutProperties.height = 128u;
				_TransmittanceLutProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("TransmittanceLut", _TransmittanceLutProperties);
				builder.WriteTexture("TransmittanceLut");

				NewTextureProperties _MultiScatteringLutProperties{};
				_MultiScatteringLutProperties.width = 256u;
				_MultiScatteringLutProperties.height = 64u;
				_MultiScatteringLutProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("MultiScatteringLut", _MultiScatteringLutProperties);
				builder.WriteTexture("MultiScatteringLut");

				NewTextureProperties _SkyViewLutProperties{};
				_SkyViewLutProperties.width = 32u;
				_SkyViewLutProperties.height = 32u;
				_SkyViewLutProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("SkyViewLut", _SkyViewLutProperties);
				builder.WriteTexture("SkyViewLut");

				NewTextureProperties _AerialPerspectiveLutProperties{};
				_AerialPerspectiveLutProperties.width = 32u * 32u;
				_AerialPerspectiveLutProperties.height = 32u;
				_AerialPerspectiveLutProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("AerialPerspectiveLut", _AerialPerspectiveLutProperties);
				builder.WriteTexture("AerialPerspectiveLut");

				shaderManger.CreateComputeShader("TransmittanceLut",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Atmosphere/Atmosphere.hlsl";
						proxy.csEntryPoint = "TransmittanceLut";
					});

				shaderManger.CreateComputeShader("MultiScatteringLut",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Atmosphere/Atmosphere.hlsl";
						proxy.csEntryPoint = "MultiScatteringLut";
					});

				shaderManger.CreateComputeShader("SkyViewLut",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Atmosphere/Atmosphere.hlsl";
						proxy.csEntryPoint = "SkyViewLut";
					});

				shaderManger.CreateComputeShader("AerialPerspectiveLut",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Atmosphere/Atmosphere.hlsl";
						proxy.csEntryPoint = "AerialPerspectiveLut";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* transmittanceLut = resourceStorage->GetResourceByName("TransmittanceLut")->GetTexture();
				auto* multiScatteringLut = resourceStorage->GetResourceByName("MultiScatteringLut")->GetTexture();
				auto* skyViewLut = resourceStorage->GetResourceByName("SkyViewLut")->GetTexture();
				auto* aerialPerspectiveLut = resourceStorage->GetResourceByName("AerialPerspectiveLut")->GetTexture();
				
				auto& transmittanceLutDesc = transmittanceLut->GetResourceFormat().GetTextureDesc();
				auto& multiScatteringLutDesc = multiScatteringLut->GetResourceFormat().GetTextureDesc();
				auto& skyViewLutDesc = skyViewLut->GetResourceFormat().GetTextureDesc();
				auto& aerialPerspectiveLutDesc = aerialPerspectiveLut->GetResourceFormat().GetTextureDesc();

				atmosphereBuilderData.transmittanceLutIndex = transmittanceLut->GetUADescriptor()->GetHeapIndex();
				atmosphereBuilderData.transmittanceLutSize = Math::Vector2{ (float)transmittanceLutDesc.height, (float)transmittanceLutDesc.width };
				atmosphereBuilderData.multiScatteringLutIndex = multiScatteringLut->GetUADescriptor()->GetHeapIndex();
				atmosphereBuilderData.multiScatteringLutSize = Math::Vector2{ (float)multiScatteringLutDesc.height, (float)multiScatteringLutDesc.width };
				atmosphereBuilderData.skyViewLutIndex = skyViewLut->GetUADescriptor()->GetHeapIndex();
				atmosphereBuilderData.skyViewLutSize = Math::Vector2{ (float)skyViewLutDesc.height, (float)skyViewLutDesc.width };
				atmosphereBuilderData.aerialPerspectiveLutIndex = aerialPerspectiveLut->GetUADescriptor()->GetHeapIndex();
				atmosphereBuilderData.aerialPerspectiveLutSize = Math::Vector2{ (float)aerialPerspectiveLutDesc.height, (float)aerialPerspectiveLutDesc.width };

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(AtmospherePass::AtmosphereBuilderData), 256u);
				memcpy(passDataAlloc.cpuAddress, &atmosphereBuilderData, sizeof(AtmospherePass::AtmosphereBuilderData));

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputeRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetComputeRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				
				commandBuffer.SetComputePipelineState("TransmittanceLut");
				commandBuffer.Dispatch(transmittanceLutDesc.width / smThreadSizeInGroup, transmittanceLutDesc.height / smThreadSizeInGroup, 1u);
				
				commandBuffer.SetComputePipelineState("MultiScatteringLut");
				commandBuffer.Dispatch(multiScatteringLutDesc.width / smThreadSizeInGroup, multiScatteringLutDesc.height / smThreadSizeInGroup, 1u);

				commandBuffer.SetComputePipelineState("SkyViewLut");
				commandBuffer.Dispatch(skyViewLutDesc.width / smThreadSizeInGroup, skyViewLutDesc.height / smThreadSizeInGroup, 1u);

				commandBuffer.SetComputePipelineState("AerialPerspectiveLut");
				commandBuffer.Dispatch(aerialPerspectiveLutDesc.width / smThreadSizeInGroup, aerialPerspectiveLutDesc.height / smThreadSizeInGroup, 1u);
			});
	}

	void AtmospherePass::AddRendererPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass("AtmosphereRendererPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadTexture("TransmittanceLut", ShaderAccessFlag::PixelShader);
				builder.ReadTexture("SkyViewLut", ShaderAccessFlag::PixelShader);

				builder.WriteRenderTarget("ShadingResult");
				builder.WriteDepthStencil("DepthStencil");

				shaderManger.CreateGraphicsShader("SkyboxRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Atmosphere/Skybox.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R16G16B16A16_FLOAT
						};
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* shadingResult = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* depthStencil = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();
				auto* transmittanceLut = resourceStorage->GetResourceByName("TransmittanceLut")->GetTexture();
				auto* skyViewLut = resourceStorage->GetResourceByName("SkyViewLut")->GetTexture();

				atmosphereRendererData.transmittanceLutIndex = transmittanceLut->GetSRDescriptor()->GetHeapIndex();
				atmosphereRendererData.skyViewLutIndex = skyViewLut->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(AtmospherePass::AtmosphereBuilderData), 256u);
				memcpy(passDataAlloc.cpuAddress, &atmosphereBuilderData, sizeof(AtmospherePass::AtmosphereBuilderData));

				uint16_t width = static_cast<uint16_t>(finalOutputDesc.width);
				uint16_t height = static_cast<uint16_t>(finalOutputDesc.height);
				commandBuffer.SetRenderTargets(
					{
						shadingResult,
					},
					depthStencil);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("SkyboxRenderer");
				commandBuffer.SetGraphicsRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetGraphicsRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				commandBuffer.SetVertexBuffer(0u, cubeMesh->GetVertexBuffer());
				commandBuffer.SetIndexBuffer(cubeMesh->GetIndexBuffer());
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1u, 0u, 0u, 0u);
			});
	}

	void AtmospherePass::InitializePass(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		std::vector<Vertex> vertices;
		vertices.resize(8u);
		vertices[0].position = Math::Vector3{ -1.0f, -1.0f, 1.0f };
		vertices[1].position = Math::Vector3{ 1.0f, -1.0f, 1.0f };
		vertices[2].position = Math::Vector3{ 1.0f, 1.0f, 1.0f };
		vertices[3].position = Math::Vector3{ -1.0f, 1.0f, 1.0f };
		vertices[4].position = Math::Vector3{ -1.0f, -1.0f, -1.0f };
		vertices[5].position = Math::Vector3{ 1.0f, -1.0f, -1.0f };
		vertices[6].position = Math::Vector3{ 1.0f, 1.0f, -1.0f };
		vertices[7].position = Math::Vector3{ -1.0f, 1.0f, -1.0f };

		std::vector<uint32_t> indices = {
			2, 1, 0, 0, 3, 2, // Front
			5, 6, 7, 7, 4, 5, // Back
			3, 0, 4, 4, 7, 3, // Left
			6, 5, 1, 1, 2, 6, // Top
			6, 2, 3, 3, 7, 6, // Right
			0, 1, 5, 5, 4, 0, // Bottom
		};

		Renderer::BufferDesc vbDesc{};
		vbDesc.stride = sizeof(Renderer::Vertex);
		vbDesc.size = vbDesc.stride * vertices.size();
		vbDesc.usage = GHL::EResourceUsage::Default;

		Renderer::BufferDesc ibDesc{};
		ibDesc.stride = sizeof(uint32_t);
		ibDesc.size = ibDesc.stride * indices.size();
		ibDesc.usage = GHL::EResourceUsage::Default;

		cubeMesh = std::make_unique<Renderer::Mesh>(
			device,
			ResourceFormat{ device, vbDesc },
			ResourceFormat{ device, ibDesc },
			nullptr,
			nullptr);

		cubeMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
		cubeMesh->GetVertexBuffer()->SetDebugName("CubeMeshVertexBuffer");
		cubeMesh->GetIndexBuffer()->SetDebugName("CubeMeshIndexBuffer");
	}

}