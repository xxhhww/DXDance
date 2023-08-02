#include "Renderer/PlaceObjectOnTerrainPass.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	struct IndirectDispatch {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_DISPATCH_ARGUMENTS  dispatchArguments;
		uint32_t pad1;
	};

	void PlaceObjectOnTerrainPass::AddPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		// 计算安置点的Y轴坐标
		renderGraph.AddPass(
			"PlaceObjectOnTerrain",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _GrassPlacementBufferProperties{};
				_GrassPlacementBufferProperties.stride = sizeof(Placement);
				_GrassPlacementBufferProperties.size = smMaxPlacementSize * _GrassPlacementBufferProperties.stride;
				_GrassPlacementBufferProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				builder.DeclareBuffer("GrassPlacementBuffer", _GrassPlacementBufferProperties);
				builder.WriteBuffer("GrassPlacementBuffer");

				NewBufferProperties _PlaceObjectOnTerrainIndirectArgsProperties{};
				_PlaceObjectOnTerrainIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_PlaceObjectOnTerrainIndirectArgsProperties.size = 1u * _PlaceObjectOnTerrainIndirectArgsProperties.stride;
				_PlaceObjectOnTerrainIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_PlaceObjectOnTerrainIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("PlaceObjectOnTerrainIndirectArgs", _PlaceObjectOnTerrainIndirectArgsProperties);
				builder.WriteCopyDstBuffer("PlaceObjectOnTerrainIndirectArgs");

				shaderManger.CreateComputeShader("PlaceObjectOnTerrain",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/ObjectOnTerrain/PlaceObjectOnTerrain.hlsl";
					});

				commandSignatureManger.CreateCommandSignature("PlaceObjectOnTerrain",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectDispatchArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDispatch));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* grassPlacementBuffer = resourceStorage->GetResourceByName("GrassPlacementBuffer")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("PlaceObjectOnTerrainIndirectArgs")->GetBuffer();
				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* terrainNormalMap = resourceStorage->GetResourceByName("TerrainNormalMap")->GetTexture();

				placeObjectOnTerrainPassData.grassPlacementBufferIndex0 = grassPlacementBuffer->GetUADescriptor()->GetHeapIndex();
				placeObjectOnTerrainPassData.heightMapIndex = terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
				placeObjectOnTerrainPassData.normalMapIndex = terrainNormalMap->GetSRDescriptor()->GetHeapIndex();
				placeObjectOnTerrainPassData.placementSizePerAxis = smPlacementSizePerAxis;

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(PlaceObjectOnTerrainPassData));
				memcpy(passDataAlloc.cpuAddress, &placeObjectOnTerrainPassData, sizeof(PlaceObjectOnTerrainPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = (smPlacementSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountY = (smPlacementSizePerAxis / smThreadSizeInGroup);
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				// 更新placementBuffer与indirectArgs
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(grassPlacementBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.ClearCounterBuffer(grassPlacementBuffer, 0u);

				barrierBatch = commandBuffer.TransitionImmediately(grassPlacementBuffer, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(grassPlacementBuffer->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("PlaceObjectOnTerrain");
				commandBuffer.ExecuteIndirect("PlaceObjectOnTerrain", indirectArgs, 1u);
			}
			);
	}

	void PlaceObjectOnTerrainPass::InitializePass(RenderEngine* renderEngine) {
	}

}