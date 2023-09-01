#include "Renderer/TerrainSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RvtUpdater.h"
#include "Renderer/RvtTiledTexture.h"
#include "Renderer/FixedTextureHelper.h"
#include "Renderer/RenderGraphBuilder.h"

#include "ECS/Entity.h"
#include "ECS/CTransform.h"
#include "ECS/CCamera.h"

namespace Renderer {

	struct IndirectDispatch {
		D3D12_GPU_VIRTUAL_ADDRESS frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS passDataAddress;
		D3D12_DISPATCH_ARGUMENTS  dispatchArguments;
		uint32_t pad1;
	};

	struct IndirectDrawIndexed {
		D3D12_GPU_VIRTUAL_ADDRESS    frameDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    passDataAddress;
		D3D12_GPU_VIRTUAL_ADDRESS    lightDataAddress;
		D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW      indexBufferView;
		D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
	};

	TerrainSystem::TerrainSystem(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine)
	, mQueuedReadbacks(mRenderEngine->mFrameTracker->GetMaxSize()) {
	}

	TerrainSystem::~TerrainSystem() {
		if (mRvtUpdater != nullptr) {
			delete mRvtUpdater;
			delete mRvtTiledTexture;
		}
	}

	void TerrainSystem::Initialize(RenderEngine* renderEngine) {
		mRenderEngine = renderEngine;

		{
			mRvtUpdater = new RvtUpdater(this);
			mRvtTiledTexture = new RvtTiledTexture(this);
		}

		auto* device = renderEngine->mDevice.get();
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* frameTracker = renderEngine->mFrameTracker.get();
		auto* resourceStorage = renderEngine->mPipelineResourceStorage;
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		auto* copyDsQueue = renderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = renderEngine->mUploaderEngine->GetCopyFence();

		auto& finalOutputDesc =
			renderGraph->GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		{
			// 创建TerrainFeedback
			TextureDesc _TerrainFeedbackDesc{};
			_TerrainFeedbackDesc.width = finalOutputDesc.width;
			_TerrainFeedbackDesc.height = finalOutputDesc.height;
			_TerrainFeedbackDesc.format = DXGI_FORMAT_R8G8B8A8_UINT;
			_TerrainFeedbackDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;
			_TerrainFeedbackDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mTerrainFeedbackMap = resourceAllocator->Allocate(
				device, _TerrainFeedbackDesc, descriptorAllocator, nullptr
			);
			mTerrainFeedbackMap->SetDebugName("TerrainFeedback");

			// 导入至RenderGraph供RenderPass使用
			renderGraph->ImportResource("TerrainFeedback", mTerrainFeedbackMap);
			resourceStateTracker->StartTracking(mTerrainFeedbackMap);
		}

		{
			// 创建Readback
			BufferDesc _TerrainReadbackDesc{};
			_TerrainReadbackDesc.size = GetRequiredIntermediateSize(mTerrainFeedbackMap->D3DResource(), 0, 1);
			_TerrainReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_TerrainReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_TerrainReadbackDesc.expectedState = _TerrainReadbackDesc.initialState;
			mTerrainReadbackBuffers.resize(frameTracker->GetMaxSize());
			for (uint32_t i = 0; i < mTerrainReadbackBuffers.size(); i++) {
				mTerrainReadbackBuffers[i] = resourceAllocator->Allocate(
					device, _TerrainReadbackDesc, descriptorAllocator, nullptr
				);
			}
		}

		// Load PatchMesh From Memory
		{
			uint32_t size = 16u;
			float sizePerGrid = 0.5f;
			float totalMeterSize = size * sizePerGrid;
			float gridCount = size * size;
			float triangleCount = gridCount * 2u;

			float vOffset = -totalMeterSize * 0.5f;

			std::vector<Vertex> vertices;
			float uvStrip = 1.0f / size;
			for (uint32_t z = 0u; z <= size; z++) {
				for (uint32_t x = 0u; x <= size; x++) {
					vertices.emplace_back(
						Math::Vector3{ vOffset + x * 0.5f, 0u, vOffset + z * 0.5f },
						Math::Vector2{ x * uvStrip, z * uvStrip },
						Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{}
					);
				}
			}

			std::vector<uint32_t> indices;
			indices.resize(triangleCount * 3u);
			for (uint32_t gridIndex = 0u; gridIndex < gridCount; gridIndex++) {
				uint32_t offset = gridIndex * 6u;
				uint32_t vIndex = (gridIndex / size) * (size + 1u) + (gridIndex % size);

				indices[offset] = vIndex;
				indices[offset + 1u] = vIndex + size + 1u;
				indices[offset + 2u] = vIndex + 1u;
				indices[offset + 3u] = vIndex + 1u;
				indices[offset + 4u] = vIndex + size + 1u;
				indices[offset + 5u] = vIndex + size + 2u;
			}

			Renderer::BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Renderer::Vertex);
			vbDesc.size = vbDesc.stride * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			Renderer::BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = ibDesc.stride * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			patchMesh = std::make_unique<Renderer::Mesh>(
				device,
				ResourceFormat{ device, vbDesc },
				ResourceFormat{ device, ibDesc },
				nullptr,
				nullptr);

			patchMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
		}

		// Load MinMaxHeightMap From Memory(目前读取的DDS文件，后续将DDS转换为XET)
		{
			minmaxHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/MinMaxHeightMap_4096H.dds");
			resourceStateTracker->StartTracking(minmaxHeightMap);
		}

		// Load HeightMap From File
		{
			heightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/HeightMap.png"
			);
			resourceStateTracker->StartTracking(heightMap);
			resourceStorage->ImportResource("TerrainHeightMap", heightMap);
		}

		// Load NormalMap From File
		{
			normalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/NormalMap_4096H.png");
			resourceStateTracker->StartTracking(normalMap);
			resourceStorage->ImportResource("TerrainNormalMap", normalMap);
		}

		// Load Grass Texture
		{
			grassAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Grass_Albedo.png");
			resourceStateTracker->StartTracking(grassAlbedoMap);
			resourceStorage->ImportResource("GrassAlbedoMap", grassAlbedoMap);

			grassNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Grass_Normal.png");
			resourceStateTracker->StartTracking(grassNormalMap);
			resourceStorage->ImportResource("GrassNormalMap", grassNormalMap);

			grassHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Grass_Height.png");
			resourceStateTracker->StartTracking(grassHeightMap);
			resourceStorage->ImportResource("GrassHeightMap", grassHeightMap);
		}

		// Load Mud Texture
		{
			mudAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Mud_Albedo.png");
			resourceStateTracker->StartTracking(mudAlbedoMap);
			resourceStorage->ImportResource("MudAlbedoMap", mudAlbedoMap);

			mudNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Mud_Normal.png");
			resourceStateTracker->StartTracking(mudNormalMap);
			resourceStorage->ImportResource("MudNormalMap", mudNormalMap);

			mudHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Mud_Height.png");
			resourceStateTracker->StartTracking(mudHeightMap);
			resourceStorage->ImportResource("MudHeightMap", mudHeightMap);
		}

		// Load Cliff Texture
		{
			cliffAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Cliff_Albedo.png");
			resourceStateTracker->StartTracking(cliffAlbedoMap);
			resourceStorage->ImportResource("CliffAlbedoMap", cliffAlbedoMap);

			cliffNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Cliff_Normal.png");
			resourceStateTracker->StartTracking(cliffNormalMap);
			resourceStorage->ImportResource("CliffNormalMap", cliffNormalMap);

			cliffHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Cliff_Height.png");
			resourceStateTracker->StartTracking(cliffHeightMap);
			resourceStorage->ImportResource("CliffHeightMap", cliffHeightMap);
		}

		// Load Snow Texture
		{
			snowAlbedoMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Snow_Albedo.png");
			resourceStateTracker->StartTracking(snowAlbedoMap);
			resourceStorage->ImportResource("SnowAlbedoMap", snowAlbedoMap);

			snowNormalMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Snow_Normal.png");
			resourceStateTracker->StartTracking(snowNormalMap);
			resourceStorage->ImportResource("SnowNormalMap", snowNormalMap);

			snowHeightMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/Snow_Height.png");
			resourceStateTracker->StartTracking(snowHeightMap);
			resourceStorage->ImportResource("SnowHeightMap", snowHeightMap);
		}

		// Load SplatMap From File
		{
			splatMap = FixedTextureHelper::LoadFromFile(
				device, descriptorAllocator, resourceAllocator, copyDsQueue, copyFence,
				"E:/MyProject/DXDance/Resources/Textures/Terrain/SplatMap.dds");
			resourceStateTracker->StartTracking(splatMap.Get());
			resourceStorage->ImportResource("SplatMap", splatMap);
		}
	}

	void TerrainSystem::AddPass(RenderEngine* renderEngine) {
		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* frameTracker = renderEngine->mFrameTracker.get();

		uint32_t _MaxNodeListSize = 200u;
		uint32_t _TmpNodeListSize = 50u;

		UpdateNodeAndLodDescriptorArray();

		renderGraph->AddPass(
			"TraverseQuadTree",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewBufferProperties _TraverseQuadTreeIndirectArgsProperties;
				_TraverseQuadTreeIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_TraverseQuadTreeIndirectArgsProperties.size = sizeof(IndirectDispatch);
				_TraverseQuadTreeIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_TraverseQuadTreeIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("TraverseQuadTreeIndirectArgs", _TraverseQuadTreeIndirectArgsProperties);
				builder.WriteCopyDstBuffer("TraverseQuadTreeIndirectArgs");

				NewBufferProperties _ConsumeNodeListProperties;
				_ConsumeNodeListProperties.stride = sizeof(uint32_t) * 2u;	// uint2 x,y
				_ConsumeNodeListProperties.size = _TmpNodeListSize * _ConsumeNodeListProperties.stride;
				_ConsumeNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_ConsumeNodeListProperties.aliased = false;
				builder.DeclareBuffer("ConsumeNodeList", _ConsumeNodeListProperties);
				builder.WriteBuffer("ConsumeNodeList");

				NewBufferProperties _AppendNodeListProperties;
				_AppendNodeListProperties.stride = sizeof(uint32_t) * 2u;	// uint2 x,y
				_AppendNodeListProperties.size = _TmpNodeListSize * _AppendNodeListProperties.stride;
				_AppendNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_AppendNodeListProperties.aliased = false;
				builder.DeclareBuffer("AppendNodeList", _AppendNodeListProperties);
				builder.WriteBuffer("AppendNodeList");

				NewBufferProperties _FinalNodeListProperties;
				_FinalNodeListProperties.stride = sizeof(uint32_t) * 3u;	// uint3 x,y,lod
				_FinalNodeListProperties.size = _MaxNodeListSize * _FinalNodeListProperties.stride;
				_FinalNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_FinalNodeListProperties.aliased = false;
				builder.DeclareBuffer("FinalNodeList", _FinalNodeListProperties);
				builder.WriteBuffer("FinalNodeList");

				NewBufferProperties _NearbyNodeListProperties;
				_NearbyNodeListProperties.stride = sizeof(uint32_t) * 3u;	// uint3 x,y,lod
				_NearbyNodeListProperties.size = _MaxNodeListSize * _NearbyNodeListProperties.stride;
				_NearbyNodeListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_NearbyNodeListProperties.aliased = false;
				builder.DeclareBuffer("NearbyNodeList", _NearbyNodeListProperties);
				builder.WriteBuffer("NearbyNodeList");

				NewBufferProperties _NodeDescriptorListProperties;
				_NodeDescriptorListProperties.stride = sizeof(NodeDescriptor);
				_NodeDescriptorListProperties.size = nodeDescriptors.size() * _NodeDescriptorListProperties.stride;
				_NodeDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_NodeDescriptorListProperties.aliased = false;
				builder.DeclareBuffer("NodeDescriptorList", _NodeDescriptorListProperties);
				builder.WriteBuffer("NodeDescriptorList");

				NewBufferProperties _LODDescriptorListProperties;
				_LODDescriptorListProperties.stride = sizeof(LODDescriptor);
				_LODDescriptorListProperties.size = lodDescriptors.size() * _LODDescriptorListProperties.stride;
				_LODDescriptorListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_LODDescriptorListProperties.aliased = false;
				builder.DeclareBuffer("LODDescriptorList", _LODDescriptorListProperties);
				builder.WriteBuffer("LODDescriptorList");

				shaderManger.CreateComputeShader("TraverseQuadTree",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainBuilder.hlsl";
						proxy.csEntryPoint = "TraverseQuadTree";
					});

				commandSignatureManger.CreateCommandSignature("TraverseQuadTree",
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
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* consumeNodeList = resourceStorage->GetResourceByName("ConsumeNodeList")->GetBuffer();
				auto* appendNodeList = resourceStorage->GetResourceByName("AppendNodeList")->GetBuffer();
				auto* finalNodeList = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* nearbyNodeList = resourceStorage->GetResourceByName("NearbyNodeList")->GetBuffer();
				auto* nodeDescriptorList = resourceStorage->GetResourceByName("NodeDescriptorList")->GetBuffer();
				auto* lodDescriptorList = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("TraverseQuadTreeIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("TraverseQuadTree");

				terrainBuilderPassData.worldMeterSize = worldMeterSize;
				terrainBuilderPassData.heightScale = worldHeightScale;
				terrainBuilderPassData.currPassLOD = maxLOD;
				terrainBuilderPassData.consumeNodeListIndex = consumeNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.appendNodeListIndex = appendNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.finalNodeListIndex = finalNodeList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.nodeDescriptorListIndex = nodeDescriptorList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.lodDescriptorListIndex = lodDescriptorList->GetUADescriptor()->GetHeapIndex();
				terrainBuilderPassData.minmaxHeightMapIndex = minmaxHeightMap->GetSRDescriptor()->GetHeapIndex();
				terrainBuilderPassData.nearbyNodeListIndex = nearbyNodeList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainSystem::TerrainBuilderPassData), 256u);
				memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainSystem::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = maxLODNodeList.size();
				indirectDispatch.dispatchArguments.ThreadGroupCountY = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};

				if (!isInitialized) {
					barrierBatch = commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::CopyDestination);
					barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::CopyDestination);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.UploadBufferRegion(nodeDescriptorList, 0u,
						nodeDescriptors.data(), nodeDescriptors.size() * sizeof(TerrainSystem::NodeDescriptor)
					);
					commandBuffer.UploadBufferRegion(lodDescriptorList, 0u,
						lodDescriptors.data(), lodDescriptors.size() * sizeof(TerrainSystem::LODDescriptor)
					);

					isInitialized = true;
				}


				barrierBatch = commandBuffer.TransitionImmediately(nodeDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(lodDescriptorList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(nearbyNodeList, GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(nearbyNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(minmaxHeightMap.Get(), GHL::EResourceState::NonPixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.UploadBufferRegion(consumeNodeList, 0u,
					maxLODNodeList.data(), maxLODNodeList.size() * sizeof(TerrainSystem::NodeLocation)
				);
				commandBuffer.ClearCounterBuffer(consumeNodeList, maxLODNodeList.size());

				commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
				commandBuffer.ClearCounterBuffer(finalNodeList, 0u);
				commandBuffer.ClearCounterBuffer(nearbyNodeList, 0u);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("TraverseQuadTree");
				for (int32_t lod = maxLOD; lod >= 0; lod--) {
					if (lod != maxLOD) {
						// 更新TerrainBuilderPassData
						uint32_t tempValue = terrainBuilderPassData.consumeNodeListIndex;
						terrainBuilderPassData.consumeNodeListIndex = terrainBuilderPassData.appendNodeListIndex;
						terrainBuilderPassData.appendNodeListIndex = tempValue;
						terrainBuilderPassData.currPassLOD = lod;

						passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainSystem::TerrainBuilderPassData), 256u);
						memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainSystem::TerrainBuilderPassData));

						// 更新IndirectArgs
						barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::CopyDestination);
						barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
						barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
						commandBuffer.FlushResourceBarrier(barrierBatch);

						commandBuffer.UploadBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 1u, &passDataAlloc.gpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
						commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, consumeNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));
						commandBuffer.ClearCounterBuffer(appendNodeList, 0u);
					}

					// ExecuteIndirect
					barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
					barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(consumeNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(appendNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(appendNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(finalNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(nearbyNodeList, GHL::EResourceState::UnorderedAccess);
					barrierBatch += commandBuffer.TransitionImmediately(nearbyNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
					commandBuffer.FlushResourceBarrier(barrierBatch);

					commandBuffer.ExecuteIndirect("TraverseQuadTree", indirectArgs, 1u);

					Buffer* tempBuffer = consumeNodeList;
					consumeNodeList = appendNodeList;
					appendNodeList = tempBuffer;
				}
			}
			);

		renderGraph->AddPass(
			"BuildPatches",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadBuffer("LODDescriptorList", ShaderAccessFlag::NonPixelShader);
				builder.ReadBuffer("FinalNodeList", ShaderAccessFlag::NonPixelShader);

				NewBufferProperties _CulledPatchListProperties{};
				_CulledPatchListProperties.stride = sizeof(TerrainSystem::RenderPatch);
				_CulledPatchListProperties.size = _MaxNodeListSize * _CulledPatchListProperties.stride * 64;
				_CulledPatchListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_CulledPatchListProperties.aliased = false;
				builder.DeclareBuffer("CulledPatchList", _CulledPatchListProperties);
				builder.WriteBuffer("CulledPatchList");

				NewBufferProperties _BuildPatchesIndirectArgsProperties{};
				_BuildPatchesIndirectArgsProperties.stride = sizeof(IndirectDispatch);
				_BuildPatchesIndirectArgsProperties.size = sizeof(IndirectDispatch);
				_BuildPatchesIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_BuildPatchesIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("BuildPatchesIndirectArgs", _BuildPatchesIndirectArgsProperties);
				builder.WriteCopyDstBuffer("BuildPatchesIndirectArgs");

				shaderManger.CreateComputeShader("BuildPatches",
					[](ComputeStateProxy& proxy) {
						proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainBuilder.hlsl";
						proxy.csEntryPoint = "BuildPatches";
					});

				commandSignatureManger.CreateCommandSignature("BuildPatches",
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
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* lodDescriptorList = resourceStorage->GetResourceByName("LODDescriptorList")->GetBuffer();
				auto* finalNodeList = resourceStorage->GetResourceByName("FinalNodeList")->GetBuffer();
				auto* culledPatchList = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("BuildPatchesIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("BuildPatches");

				terrainBuilderPassData.worldMeterSize = worldMeterSize;
				terrainBuilderPassData.heightScale = worldHeightScale;
				terrainBuilderPassData.lodDescriptorListIndex = lodDescriptorList->GetSRDescriptor()->GetHeapIndex();
				terrainBuilderPassData.finalNodeListIndex = finalNodeList->GetSRDescriptor()->GetHeapIndex();
				terrainBuilderPassData.culledPatchListIndex = culledPatchList->GetUADescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainSystem::TerrainBuilderPassData));
				memcpy(passDataAlloc.cpuAddress, &terrainBuilderPassData, sizeof(TerrainSystem::TerrainBuilderPassData));

				IndirectDispatch indirectDispatch{};
				indirectDispatch.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDispatch.passDataAddress = passDataAlloc.gpuAddress;
				indirectDispatch.dispatchArguments.ThreadGroupCountX = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountY = 1u;
				indirectDispatch.dispatchArguments.ThreadGroupCountZ = 1u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDispatch, sizeof(IndirectDispatch));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);
				commandBuffer.CopyBufferRegion(indirectArgs, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 2u, finalNodeList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				commandBuffer.ClearCounterBuffer(culledPatchList, 0u);

				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(finalNodeList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.SetComputeRootSignature();
				commandBuffer.SetComputePipelineState("BuildPatches");
				commandBuffer.ExecuteIndirect("BuildPatches", indirectArgs, 1u);
			}
			);

		renderGraph->AddPass(
			"TerrainRenderer",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.ReadBuffer("CulledPatchList", ShaderAccessFlag::PixelShader);
				builder.WriteRenderTarget("ShadingResult");
				builder.WriteRenderTarget("NormalRoughness");
				builder.WriteRenderTarget("ScreenVelocity");
				builder.WriteRenderTarget("TerrainFeedback");
				builder.WriteDepthStencil("DepthStencil");

				NewBufferProperties _TerrainRendererIndirectArgsProperties{};
				_TerrainRendererIndirectArgsProperties.stride = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.size = sizeof(IndirectDrawIndexed);
				_TerrainRendererIndirectArgsProperties.miscFlag = GHL::EBufferMiscFlag::IndirectArgs;
				_TerrainRendererIndirectArgsProperties.aliased = false;
				builder.DeclareBuffer("TerrainRendererIndirectArgs", _TerrainRendererIndirectArgsProperties);
				builder.WriteCopyDstBuffer("TerrainRendererIndirectArgs");

				shaderManger.CreateGraphicsShader("TerrainRenderer",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainRenderer_ForwardPlus.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16_FLOAT,
							DXGI_FORMAT_R8G8B8A8_UINT	// TerrainFeedback
						};
					});

				commandSignatureManger.CreateCommandSignature("TerrainRenderer",
					[&](GHL::CommandSignature& proxy) {
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 0u });
						proxy.AddIndirectArgument(GHL::IndirectConstantBufferViewArgument{ 1u });
						proxy.AddIndirectArgument(GHL::IndirectShaderResourceViewArgument{ 2u });
						proxy.AddIndirectArgument(GHL::IndirectVertexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectIndexBufferViewArgument{});
						proxy.AddIndirectArgument(GHL::IndirectDrawIndexedArgument{});
						proxy.SetRootSignature(shaderManger.GetBaseD3DRootSignature());
						proxy.SetByteStride(sizeof(IndirectDrawIndexed));
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* shadingResult = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* normalRoughness = resourceStorage->GetResourceByName("NormalRoughness")->GetTexture();
				auto* screenVelocity = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
				auto* depthStencil = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();
				auto* terrainFeedback = resourceStorage->GetResourceByName("TerrainFeedback")->GetTexture();
				auto* culledPatchList = resourceStorage->GetResourceByName("CulledPatchList")->GetBuffer();
				auto* indirectArgs = resourceStorage->GetResourceByName("TerrainRendererIndirectArgs")->GetBuffer();

				auto* cmdSig = commandSignatureManger->GetD3DCommandSignature("TerrainRenderer");

				// 更新Rvt参数
				float rvtRadius      = mRvtUpdater->GetRvtRadius();
				uint32_t tableSize   = mRvtUpdater->GetTableSize();
				uint32_t maxMipLevel = mRvtUpdater->GetMaxMipLevel();
				uint32_t tileSize    = mRvtTiledTexture->GetTileSize();
				Math::Int2 fixedCenter = mRvtUpdater->GetFixedCenter(mRvtUpdater->GetFixedPos(
					renderContext.resourceStorage->rootConstantsPerFrame.currentRenderCamera.position));
				mRealTotalRect = Math::Vector4{ fixedCenter.x - rvtRadius, fixedCenter.y - rvtRadius, 2 * rvtRadius, 2 * rvtRadius };
				
				terrainRendererPassData.vtFeedbackParams = Math::Vector4{ (float)tableSize, (float)(tableSize * tileSize), (float)maxMipLevel - 1.0f, 0.0f };
				terrainRendererPassData.vtRealRect = mRealTotalRect;

				terrainRendererPassData.worldMeterSize = worldMeterSize;
				terrainRendererPassData.heightScale = worldHeightScale;
				terrainRendererPassData.culledPatchListIndex = culledPatchList->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.heightMapIndex = heightMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.normalMapIndex = normalMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.splatMapIndex = splatMap->GetSRDescriptor()->GetHeapIndex();

				terrainRendererPassData.rChannelAlbedoMapIndex = grassAlbedoMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.rChannelNormalMapIndex = grassNormalMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.rChannelHeightMapIndex = grassHeightMap->GetSRDescriptor()->GetHeapIndex();
				// terrainRendererPassData.rChannelRoughnessMapIndex = grassRoughnessMap->GetSRDescriptor()->GetHeapIndex();

				terrainRendererPassData.gChannelAlbedoMapIndex = mudAlbedoMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.gChannelNormalMapIndex = mudNormalMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.gChannelHeightMapIndex = mudHeightMap->GetSRDescriptor()->GetHeapIndex();
				// terrainRendererPassData.gChannelRoughnessMapIndex = mudRoughnessMap->GetSRDescriptor()->GetHeapIndex();

				terrainRendererPassData.bChannelAlbedoMapIndex = cliffAlbedoMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.bChannelNormalMapIndex = cliffNormalMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.bChannelHeightMapIndex = cliffHeightMap->GetSRDescriptor()->GetHeapIndex();
				// terrainRendererPassData.bChannelRoughnessMapIndex = cliffRoughnessMap->GetSRDescriptor()->GetHeapIndex();

				terrainRendererPassData.aChannelAlbedoMapIndex = snowAlbedoMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.aChannelNormalMapIndex = snowNormalMap->GetSRDescriptor()->GetHeapIndex();
				terrainRendererPassData.aChannelHeightMapIndex = snowHeightMap->GetSRDescriptor()->GetHeapIndex();
				// terrainRendererPassData.aChannelRoughnessMapIndex = snowRoughness->GetSRDescriptor()->GetHeapIndex();

				auto passDataAlloc = dynamicAllocator->Allocate(sizeof(TerrainSystem::TerrainRendererPassData));
				memcpy(passDataAlloc.cpuAddress, &terrainRendererPassData, sizeof(TerrainSystem::TerrainRendererPassData));

				IndirectDrawIndexed indirectDrawIndexed{};
				indirectDrawIndexed.frameDataAddress = resourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexed.lightDataAddress = resourceStorage->rootLightDataPerFrameAddress;
				indirectDrawIndexed.passDataAddress = passDataAlloc.gpuAddress;
				indirectDrawIndexed.vertexBufferView = patchMesh->GetVertexBuffer()->GetVBDescriptor();
				indirectDrawIndexed.indexBufferView = patchMesh->GetIndexBuffer()->GetIBDescriptor();
				indirectDrawIndexed.drawIndexedArguments.IndexCountPerInstance = patchMesh->GetIndexCount();
				indirectDrawIndexed.drawIndexedArguments.InstanceCount = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartIndexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.BaseVertexLocation = 0u;
				indirectDrawIndexed.drawIndexedArguments.StartInstanceLocation = 0u;

				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::CopySource);
				barrierBatch += commandBuffer.TransitionImmediately(heightMap.Get(), GHL::EResourceState::PixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.UploadBufferRegion(indirectArgs, 0u, &indirectDrawIndexed, sizeof(IndirectDrawIndexed));
				commandBuffer.ClearCounterBuffer(indirectArgs, 1u);

				commandBuffer.CopyBufferRegion(indirectArgs,
					sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * 3u + sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(UINT),
					culledPatchList->GetCounterBuffer(), 0u, sizeof(uint32_t));

				barrierBatch = commandBuffer.TransitionImmediately(indirectArgs, GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(indirectArgs->GetCounterBuffer(), GHL::EResourceState::IndirectArgument);
				barrierBatch += commandBuffer.TransitionImmediately(culledPatchList->GetCounterBuffer(), GHL::EResourceState::UnorderedAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				auto& shadingResultDesc = shadingResult->GetResourceFormat().GetTextureDesc();
				uint16_t width = static_cast<uint16_t>(shadingResultDesc.width);
				uint16_t height = static_cast<uint16_t>(shadingResultDesc.height);
				commandBuffer.ClearRenderTarget(terrainFeedback);
				commandBuffer.SetRenderTargets(
					{
						shadingResult,
						normalRoughness,
						screenVelocity,
						terrainFeedback,
					},
					depthStencil);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("TerrainRenderer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.ExecuteIndirect("TerrainRenderer", indirectArgs, 1u);

				// 渲染完成之后，将Feedback复制到ReadbackBuffer中
				barrierBatch = commandBuffer.TransitionImmediately(terrainFeedback, GHL::EResourceState::CopySource);
				commandBuffer.FlushResourceBarrier(barrierBatch);
				uint8_t currentFrameIndex = frameTracker->GetCurrFrameIndex();
				BufferWrap& resolvedReadback = mTerrainReadbackBuffers[currentFrameIndex];
				auto& srcDesc = mTerrainFeedbackMap->GetResourceFormat().D3DResourceDesc();
				uint32_t rowPitch = (srcDesc.Width * GHL::GetFormatStride(srcDesc.Format) + 0x0ff) & ~0x0ff;
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{ 0,
					{ srcDesc.Format, (UINT)srcDesc.Width, srcDesc.Height, srcDesc.DepthOrArraySize, rowPitch } };

				D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mTerrainFeedbackMap->D3DResource(), 0);
				D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(resolvedReadback->D3DResource(), layout);

				// TODO 判断当前帧的QueuedReadback是否仍为Fresh，如果是，则说明ProcessFeedback线程还未处理完该帧的Feedback，需要等待其不再是Fresh的状态
				const auto& currFrameAttribute = frameTracker->GetCurrFrameAttribute();
				mQueuedReadbacks.at(currFrameAttribute.frameIndex).renderFrameFenceValue = currFrameAttribute.fenceValue;
				mQueuedReadbacks.at(currFrameAttribute.frameIndex).isFresh = true;

				commandBuffer.D3DCommandList()->CopyTextureRegion(
					&dstLocation,
					0, 0, 0,
					&srcLocation,
					nullptr);
			}
			);
	}

	void TerrainSystem::FrameCompletedCallback(uint8_t frameIndex) {
		// 渲染帧完成之后，对Feedback进行回读操作
		// mRvtUpdater->SetFrameCompletedEvent();
		// mQueuedReadbacks.at(frameIndex).isFresh = true;
	}

	void TerrainSystem::UpdateNodeAndLodDescriptorArray() {
		uint32_t nodeCount{ 0u };

		lodDescriptors.resize(maxLOD + 1u);
		for (int32_t i = maxLOD; i >= 0; i--) {
			auto& lodDescriptor = lodDescriptors.at(i);

			uint32_t currDetailNodeSize = mostDetailNodeMeterSize * pow(2, i);
			uint32_t nodeCountPerAxis = terrainBuilderPassData.worldMeterSize.x / currDetailNodeSize;

			lodDescriptor.nodeSize = currDetailNodeSize;
			lodDescriptor.nodeStartOffset = nodeCount;
			lodDescriptor.nodeCount = nodeCountPerAxis * nodeCountPerAxis;

			nodeCount += lodDescriptor.nodeCount;
		}
		nodeDescriptors.resize(nodeCount);

		for (uint32_t i = 0; i < 5; i++) {
			for (uint32_t j = 0; j < 5; j++) {
				NodeLocation nodeLocation;
				nodeLocation.x = i;
				nodeLocation.y = j;
				maxLODNodeList.push_back(nodeLocation);
			}
		}

		isInitialized = false;
	}

}