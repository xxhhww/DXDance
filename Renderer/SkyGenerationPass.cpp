#include "SkyGenerationPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CLight.h"
#include "ECS/CSky.h"

namespace Renderer {

	void SkyGenerationPass::AddPass(RenderGraph& renderGraph) {
		renderGraph.AddPass(
			"SkyGenerationPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewTextureProperties _SkyLuminanceProperties{};
				_SkyLuminanceProperties.width = 1024u;
				_SkyLuminanceProperties.height = 1024u;
				_SkyLuminanceProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				builder.DeclareTexture("SkyLuminance", _SkyLuminanceProperties);
				builder.WriteTexture("SkyLuminance");

				shaderManger.CreateComputeShader("SkyGenerationPass",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/SkyGenerationPass.hlsl";
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* skyLuminance = resourceStorage->GetResourceByName("SkyLuminance")->GetTexture();
				auto& skyLuminanceDesc = skyLuminance->GetResourceFormat().GetTextureDesc();

				uint32_t threadGroupCountX = (skyLuminanceDesc.width  + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;
				uint32_t threadGroupCountY = (skyLuminanceDesc.height + smThreadSizeInGroup - 1u) / smThreadSizeInGroup;

				skyGenerationPassData.skyLuminanceMapIndex  = skyLuminance->GetUADescriptor()->GetHeapIndex();
				skyGenerationPassData.skyLuminanceMapSizeX  = skyLuminanceDesc.width;
				skyGenerationPassData.skyLuminanceMapSizeY  = skyLuminanceDesc.height;
				skyGenerationPassData.dispatchGroupCountX   = threadGroupCountX;
				skyGenerationPassData.dispatchGroupCountY   = threadGroupCountY;
				ECS::Entity::ForeachInCurrentThread([&](ECS::Transform& transform, ECS::Sky& sky) {
					const Math::Vector3 sunDirection = Math::Vector3{ 0.0f, 0.0f, -1.0f }.TransformAsVector(transform.worldRotation.RotationMatrix());
					skyGenerationPassData.sunDirection = sunDirection;
					auto copyState = [](GPUArHosekSkyModelState& gpuState, const ArHosekSkyModelState* cpuState) {
						for (auto i = 0; i < 3; ++i) {
							for (auto configIdx = 0; configIdx < 9; ++configIdx) {
								gpuState.configs[i][configIdx] = cpuState->configs[i][configIdx];
							}
						}
						gpuState.radiances.x = cpuState->radiances[0];
						gpuState.radiances.y = cpuState->radiances[1];
						gpuState.radiances.z = cpuState->radiances[2];
					};
					copyState(skyGenerationPassData.skyStateR, sky.skyModelStateR);
					copyState(skyGenerationPassData.skyStateG, sky.skyModelStateG);
					copyState(skyGenerationPassData.skyStateB, sky.skyModelStateB);
				});

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(SkyGenerationPass));
				memcpy(passDataAlloc.cpuAddress, &skyGenerationPassData, sizeof(SkyGenerationPass));

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("SkyGenerationPass");
				commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.Dispatch(threadGroupCountX, threadGroupCountY, 1u);
			});
	}

}