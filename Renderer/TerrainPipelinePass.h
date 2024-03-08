#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/TerrainRenderer.h"

#include "Renderer/Mesh.h"

namespace Renderer {

	class TerrainRenderer;

	class TerrainPipelinePass {
	public:
		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC;		// 用户控制的节点评估系数

			Math::Vector2 terrainMeterSize;		// 世界在XZ轴方向的大小(米)
			float         terrainHeightScale;	// Y轴缩放大小
			uint32_t      useFrustumCull;

			uint32_t currPassLOD;
			uint32_t consumeNodeListIndex;
			uint32_t appendNodeListIndex;
			uint32_t finalNodeListIndex;

			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;
			uint32_t culledPatchListIndex;
			uint32_t nodeGpuRuntimeStatesIndex;

			uint32_t nearCulledPatchListIndex;
			uint32_t farCulledPatchListIndex;
			uint32_t maxLod;
			uint32_t lodMapIndex;

			Math::Vector4 runtimeVTRealRect{};

			float sectorMeterSize;
			uint32_t useRenderCameraDebug;
			float pad2;
			float pad3;
		};

		struct TerrainRendererPassData {
			Math::Vector2 terrainMeterSize;
			float         terrainHeightScale;
			uint32_t      culledPatchListIndex;

			uint32_t nearCulledPatchListIndex;
			uint32_t farCulledPatchListIndex;
			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;

			uint32_t terrainHeightMapAtlasIndex;
			uint32_t terrainAlbedoMapAtlasIndex;
			uint32_t terrainNormalMapAtlasIndex;
			uint32_t lodDebug;

			uint32_t terrainAtlasTileCountPerAxis;
			uint32_t terrainAtlasTileWidthInPixels;
			uint32_t terrainPatchVertexCountPerAxis;
			uint32_t pageLevelDebug;

			uint32_t runtimeVTPageTableMapIndex;
			uint32_t runtimeVTAlbedoAtlasIndex;
			uint32_t runtimeVTNormalAtlasIndex;
			float    runtimeVTAtlasSize;

			Math::Vector4 runtimeVTRealRect{};

			float    runtimeVTTileCountPerAxisInPage0Level;
			float    runtimeVTMaxPageLevel;						// 理论最高的PageLevel,而不是实际最高的PageLevel
			float    tilePaddingSize;
			float    tileSizeNoPadding;

			uint32_t lodMapIndex;
			float    patchMeshGridSize;
			float    sectorMeterSize;
			float    pad3;
		};

		struct TerrainFeedbackPassData {
			Math::Vector2 terrainMeterSize;
			float         terrainHeightScale;
			uint32_t      culledPatchListIndex;

			uint32_t nodeDescriptorListIndex;
			uint32_t lodDescriptorListIndex;
			uint32_t terrainHeightMapAtlasIndex;
			uint32_t terrainAtlasTileCountPerAxis;

			uint32_t terrainAtlasTileWidthInPixels;
			uint32_t terrainPatchVertexCountPerAxis;
			uint32_t tileCountPerAxisInPage0Level;
			uint32_t scaledVirtualTextureSizeInBytesInPage0Level;

			uint32_t maxPageLevel;								// 实际最高的PageLevel
			uint32_t pageLevelBias;
			uint32_t useRenderCameraDebug;
			float pad2;

			Math::Vector4 rvtRealRect{};
		};

	public:
		TerrainPipelinePass(TerrainRenderer* renderer);
		~TerrainPipelinePass();

		void AddPass();

	private:
		void CreateGraphicsObject();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		TerrainSetting& mTerrainSetting;

		inline static uint32_t smThreadSizeInGroup = 8u;

		struct RenderPatch {
			Math::Vector2 position;
			Math::Vector2 minmaxHeight;

			uint32_t nodeLocationX;
			uint32_t nodeLocationY;
			uint32_t nodeLod;
			float pad1;

			uint32_t patchOffsetX;
			uint32_t patchOffsetY;
			float pad2;
			float pad3;

			uint32_t lodTransX;
			uint32_t lodTransY;
			uint32_t lodTransZ;
			uint32_t lodTransW;
		};
		struct NodeLocation {
			uint32_t x;
			uint32_t y;
		};
		std::vector<NodeLocation> mMaxLodNodeList;

		uint32_t   mPatchMeshVertexCountPerAxis;
		BufferWrap mPatchMeshVertexBuffer;
		BufferWrap mPatchMeshIndexBuffer;
		uint32_t   mPatchMeshIndexCount;
		float      mPatchMeshGridSize;

		TerrainBuilderPassData mTerrainBuilderPassData;
		TerrainRendererPassData mTerrainRendererPassData;
		TerrainFeedbackPassData mTerrainFeedbackPassData;
	};

}