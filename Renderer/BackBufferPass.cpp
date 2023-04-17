#include "BackBufferPass.h"
#include "RenderGraphBuilder.h"
#include "ShaderManger.h"
#include "LinearBufferAllocator.h"

namespace Renderer {

	struct Vertex {
		Math::Vector3 pos;
		Math::Vector3 normal;
		Math::Vector3 tangent;
		Math::Vector2 uv;
	};

	void BackBufferPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"BackBufferPass",
			[=](RenderGraphBuilder& builder, ShaderManger& manger) {

				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);
				builder.WriteRenderTarget("FinalOutput");

				manger.CreateGraphicsShader("BackBufferPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Shader/BackBufferPassTest.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
					});
			},
			[=](CommandListWrap& commandList, RenderContext& context) {
				
				Vertex triangleVertices[] = {
					{ { 0.0f, 0.25f, 0.0f }    },
					{ { 0.25f, -0.25f, 0.0f }  },
					{ { -0.25f, -0.25f, 0.0f } }
				};
				const UINT vertexBufferSize = sizeof(triangleVertices);
				
				auto* dynamicAllocator = context.dynamicAllocator;
				auto dynamicAllocation = dynamicAllocator->Allocate(vertexBufferSize, 4);
				memcpy(dynamicAllocation.cpuAddress, &triangleVertices, vertexBufferSize);

				D3D12_VERTEX_BUFFER_VIEW vbView{};
				vbView.BufferLocation = dynamicAllocation.gpuAddress;
				vbView.StrideInBytes = sizeof(Vertex);
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

				commandList->D3DCommandList()->RSSetViewports(1u, &viewPort);
				commandList->D3DCommandList()->RSSetScissorRects(1u, &rect);

				// commandList->D3DCommandList()->ClearRenderTargetView(cpuHandle, clearColor, 1u, &rect);
				commandList->D3DCommandList()->OMSetRenderTargets(1u, &cpuHandle, false, nullptr);

				commandList->D3DCommandList()->SetGraphicsRootSignature(shaderManger->GetBaseD3DRootSignature());
				commandList->D3DCommandList()->SetPipelineState(shader->GetD3DPipelineState());

				commandList->D3DCommandList()->IASetVertexBuffers(0u, 1u, &vbView);
				commandList->D3DCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->D3DCommandList()->DrawInstanced(3u, 1u, 0u, 0u);
				

			});
	}

}