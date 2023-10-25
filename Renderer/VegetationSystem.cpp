#include "Renderer/VegetationSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/TerrainSystem.h"

namespace Renderer {

	void VegetationSystem::Initialize(RenderEngine* renderEngine) {
		TerrainSystem* terrainSystem = renderEngine->mTerrainSystem.get();

		mDataCacher = std::make_unique<VegetationDataCache>(renderEngine);
		mDataCacher->ConfigureGrassClusterCache(smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * 1.5f, smMaxGrassBladeCountPerAxis);
		mVegetationVirtualTable = std::make_unique<VegetationVirtualTable>(mDataCacher.get(), smVisibleGrassClusterCountPerAxis, smGrassClusterMeterSize, terrainSystem->worldMeterSize);
	
		// TODO ... Load Grass Mask
	}

	/*
	* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出，由RenderEngine的Update函数调用
	*/
	void VegetationSystem::Update(const Math::Vector2& cameraPosition) {
		mNeedBakedGpuGrassClusters.clear();
		mNeedCullGpuGrassClusters.clear();

		std::vector<VegetationVirtualTable::VirtualTableCell&> changedVirtualTableCells;
		mVegetationVirtualTable->Update(cameraPosition, changedVirtualTableCells);

		auto& allVirtualTableCells = mVegetationVirtualTable->GetVirtualTable();

		// 填充两个GpuGrassClusters
		for (const auto& cell : changedVirtualTableCells) {
			mNeedBakedGpuGrassClusters.emplace_back(
				GpuGrassCluster{ cell.targetGrassClusterRect, cell.cahce->userData.grassBladeBufferIndex }
			);
		}

		for (const auto& tableRow : allVirtualTableCells) {
			for (const auto& cell : tableRow) {
				mNeedCullGpuGrassClusters.emplace_back(
					GpuGrassCluster{ cell.targetGrassClusterRect, cell.cahce->userData.grassBladeBufferIndex }
				);
			}
		}
	}

	void VegetationSystem::AddPass(RenderGraph& renderGraph) {

		/*
		* 草点的实时烘焙与剔除
		*/
		renderGraph.AddPass(
			"BakeAndCullGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("TerrainHeightMap", ShaderAccessFlag::NonPixelShader);	// 由TerrainSystem导入
				builder.ReadTexture("GrassLayerMask", ShaderAccessFlag::NonPixelShader);	// 由VegetationDataCache导入
				builder.WriteBuffer("BakedGrassBladeBuffer");								// 由VegetationDataCache导入

				NewBufferProperties _GrassClusterListProperties{};
				_GrassClusterListProperties.stride = sizeof(GpuGrassCluster);
				_GrassClusterListProperties.size = smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis * _GrassClusterListProperties.stride;
				_GrassClusterListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassClusterListProperties.aliased = false;
				builder.DeclareBuffer("GrassClusterList", _GrassClusterListProperties);
				builder.WriteBuffer("GrassClusterList");

				// 8 MB(LOD 0)
				NewBufferProperties _GrassBladeIndexListProperties{};
				_GrassBladeIndexListProperties.stride = sizeof(uint32_t);
				_GrassBladeIndexListProperties.size = smMaxGrassBladeCountPerAxis * smMaxGrassBladeCountPerAxis * smVisibleGrassClusterCountPerAxis * smVisibleGrassClusterCountPerAxis / 2u;
				_GrassBladeIndexListProperties.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
				_GrassBladeIndexListProperties.aliased = false;
				builder.DeclareBuffer("GrassBladeIndexList", _GrassBladeIndexListProperties);
				builder.WriteBuffer("GrassBladeIndexList");
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* terrainHeightMap = resourceStorage->GetResourceByName("TerrainHeightMap")->GetTexture();
				auto* grassLayerMask = resourceStorage->GetResourceByName("GrassLayerMask")->GetTexture();
				auto* bakedGrassBladeBuffer = resourceStorage->GetResourceByName("BakedGrassBladeBuffer")->GetBuffer();
				auto* grassClusterList = resourceStorage->GetResourceByName("GrassClusterList")->GetBuffer();
				auto* grassBladeIndexList = resourceStorage->GetResourceByName("GrassBladeIndexList")->GetBuffer();

				if (!mNeedBakedGpuGrassClusters.empty()) {
					// 执行草点(Grass Blade)烘焙工作
				}

				// 执行剔除工作

			});

		/*
		* 草点渲染
		*/
		renderGraph.AddPass(
			"RenderGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {

			});

	}

}