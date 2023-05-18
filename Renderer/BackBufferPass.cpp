#include "BackBufferPass.h"
#include "RenderGraphBuilder.h"
#include "ShaderManger.h"
#include "LinearBufferAllocator.h"

namespace Renderer {

	struct TempVertex {
		Math::Vector3 pos;
		Math::Vector2 uv;
		Math::Vector3 normal;
		Math::Vector3 tangent;
		Math::Vector3 bitangent;
	};

	void BackBufferPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"BackBufferPass",
			[=](RenderGraphBuilder& builder, ShaderManger& manger) {

				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);
				builder.WriteRenderTarget("FinalOutput");

				manger.CreateGraphicsShader("BackBufferPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/BackBufferPassTest.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
					});
			},
			[=](CommandListWrap& commandList, RenderContext& context) {
				auto* streamTexture = context.streamTextureManger->Request("E:/MyProject/DXDance/Renderer/media/4ktiles.xet");
				uint32_t srvIndex = streamTexture->GetInternalResource()->GetSRDescriptor()->GetHeapIndex();

				TempVertex triangleVertices[] = {
					{ { -1.0f, 1.0f, 0.0f  }, {0.0f, 0.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } },
					{ { 1.0f, -1.0f, 0.0f  }, {1.0f, 1.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } },
					{ { -1.0f, -1.0f, 0.0f }, {0.0f, 1.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } },
					{ { -1.0f, 1.0f, 0.0f  }, {0.0f, 0.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } },
					{ { 1.0f, 1.0f, 0.0f   }, {1.0f, 0.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } },
					{ { 1.0f, -1.0f, 0.0f  }, {1.0f, 1.0f}, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f }, { 0.0f, 0.25f, 0.0f } }
				};
				const UINT vertexBufferSize = sizeof(triangleVertices);
				
				auto* dynamicAllocator = context.dynamicAllocator;
				auto dynamicAllocation = dynamicAllocator->Allocate(vertexBufferSize, 256u);
				memcpy(dynamicAllocation.cpuAddress, &triangleVertices, vertexBufferSize);

				D3D12_VERTEX_BUFFER_VIEW vbView{};
				vbView.BufferLocation = dynamicAllocation.gpuAddress;
				vbView.StrideInBytes = sizeof(TempVertex);
				vbView.SizeInBytes = dynamicAllocation.size;
				
				auto* shaderManger = context.shaderManger;
				auto* shader = shaderManger->GetShader<GraphicsShader>("BackBufferPass");

				D3D12_VIEWPORT viewPort{};
				viewPort.TopLeftX = 0.0f;
				viewPort.TopLeftY = 0.0f;
				viewPort.Width = 1920.0f;
				viewPort.Height = 1080.0f;

				D3D12_RECT rect{};
				rect.left = 0.0f;
				rect.top = 0.0f;
				rect.right = 1920.0f;
				rect.bottom = 1080.0f;

				auto* resource = context.resourceStorage->GetResourceByName("FinalOutput");
				Texture* texture = static_cast<Texture*>(resource->resource);
				auto* rtv = texture->GetRTDescriptor();
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtv->GetCpuHandle();

				FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

				streamTexture->RecordClearFeedback(commandList->D3DCommandList());

				commandList->D3DCommandList()->RSSetViewports(1u, &viewPort);
				commandList->D3DCommandList()->RSSetScissorRects(1u, &rect);

				commandList->D3DCommandList()->ClearRenderTargetView(cpuHandle, clearColor, 1u, &rect);
				commandList->D3DCommandList()->OMSetRenderTargets(1u, &cpuHandle, false, nullptr);

				commandList->D3DCommandList()->SetGraphicsRootSignature(shaderManger->GetBaseD3DRootSignature());
				commandList->D3DCommandList()->SetPipelineState(shader->GetD3DPipelineState());
				commandList->D3DCommandList()->SetGraphicsRootConstantBufferView(0u, context.resourceStorage->rootConstantsPerFrameAddress);

				commandList->D3DCommandList()->IASetVertexBuffers(0u, 1u, &vbView);
				commandList->D3DCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->D3DCommandList()->DrawInstanced(6u, 1u, 0u, 0u);

				streamTexture->RecordResolve(commandList->D3DCommandList());
				streamTexture->RecordReadback(commandList->D3DCommandList());
			});
	}

}