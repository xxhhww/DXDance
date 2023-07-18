#include "Renderer/FoliagePass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	void FoliagePass::AddPass(RenderGraph& renderGraph) {
		renderGraph.AddPass(
			"FoliageRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("GBufferAlbedoMetalness");
				builder.WriteRenderTarget("GBufferPositionEmission");
				builder.WriteRenderTarget("GBufferNormalRoughness");
				builder.WriteRenderTarget("GBufferMotionVector");
				builder.WriteRenderTarget("GBufferViewDepth");
				builder.WriteDepthStencil("GBufferDepthStencil");

				shaderManger.CreateGraphicsShader("FoliageRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Foliage/FoliageRenderer.hlsl";
						proxy.gsFilepath = proxy.vsFilepath;
						proxy.psFilepath = proxy.vsFilepath;

						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R8G8B8A8_UNORM,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R32_FLOAT
						};
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;


			}
			);
	}

	void FoliagePass::InitializePass(RenderEngine* renderEngine) {
		auto* device = renderEngine->mDevice.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		// 加载placementBuffer
		{
			// 创建placementBuffer
			Renderer::BufferDesc placementBufferDesc{};
			placementBufferDesc.stride = sizeof(Placement);
			placementBufferDesc.size = placementBufferDesc.stride * smMaxPlacementSize;
			placementBufferDesc.usage = GHL::EResourceUsage::Default;
			placementBufferDesc.expectedState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::UnorderedAccess;

			placementBuffer = resourceAllocator->Allocate(device, placementBufferDesc, descriptorAllocator, nullptr);

			// 填充placementBuffer的CPU临时数据
			uint32_t placementSize = 1u;
			std::vector<Placement> placements;
			placements.resize(placementSize);
			for (uint32_t i = 0; i < placementSize; i++) {
				auto& placement = placements.at(i);
				placement.position = Math::Vector4{ i, i, i, 0 };
				placement.normal = Math::Vector4{ 0, 0, 0, 0 };
				placement.modelTrans = Math::Matrix4{};
			}

			// 将数据上传至placementBuffer
			DSTORAGE_REQUEST request = {};

			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.Memory.Source = static_cast<void*>(placements.data());
			request.Source.Memory.Size = placements.size() * sizeof(Placement);
			request.Destination.Buffer.Resource = placementBuffer->D3DResource();
			request.Destination.Buffer.Offset = 0u;
			request.Destination.Buffer.Size = placements.size() * sizeof(Placement);
			request.UncompressedSize = placements.size() * sizeof(Placement);

			copyDsQueue->EnqueueRequest(&request);
			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();

			placementBuffer->SetDebugName("FoliagePlacementBuffer");
			resourceStateTracker->StartTracking(placementBuffer);
		}

		// 加载FoliageQuadMesh
		{
			foliageQuadModel = std::make_unique<Model>(
				device, descriptorAllocator, nullptr, 
				"E:/MyProject/DXDance/Resources/Models/Vegetation/Foliage/foliage_quad_multiple.obj");
			foliageQuadModel->LoadDataFromDisk(copyDsQueue, copyFence);
			foliageQuadModel->GetFirstMesh()->GetVertexBuffer()->SetDebugName("FoliageQuadVertexBuffer");
			foliageQuadModel->GetFirstMesh()->GetIndexBuffer()->SetDebugName("FoliageQuadIndexBuffer");

			// 不在对VertexBuffer和IndexBuffer进行状态追踪
		}

	}

}