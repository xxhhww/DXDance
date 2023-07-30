#include "Renderer/FinalBarrierPass.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	/*
	struct TempVertex {
		Math::Vector3 pos;
		Math::Vector2 uv;
		Math::Vector3 normal;
		Math::Vector3 tangent;
		Math::Vector3 bitangent;
	};
	*/

	void FinalBarrierPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"FinalBarrierPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("FinalOutput");
				/*
				shaderManger.CreateGraphicsShader("BackBufferPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/BackBufferPassTest.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
					});
				*/
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
			});
	}

}