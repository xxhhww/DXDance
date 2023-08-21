#include "Renderer/SkyPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {
	
	void SkyPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass("SkyPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteDepthStencil("DepthStencil");

				shaderManger.CreateGraphicsShader("SkyPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ProceduralSky/SkyRenderer.hlsl";
						proxy.psFilepath = proxy.vsFilepath;

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
				commandBuffer.SetGraphicsPipelineState("SkyPass");
				commandBuffer.SetGraphicsRootCBV(0u, resourceStorage->rootConstantsPerFrameAddress);
				commandBuffer.SetGraphicsRootSRV(2u, resourceStorage->rootLightDataPerFrameAddress);
				commandBuffer.SetVertexBuffer(0u, cubeMesh->GetVertexBuffer());
				commandBuffer.SetIndexBuffer(cubeMesh->GetIndexBuffer());
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.DrawIndexedInstanced(cubeMesh->GetIndexCount(), 1u, 0u, 0u, 0u);
			});
	}

	void SkyPass::InitializePass(RenderEngine* renderEngine) {
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