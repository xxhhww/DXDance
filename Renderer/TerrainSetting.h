#pragma once
#include <cstdint>

namespace Renderer {

	/*
	* 地形设置
	*/
	struct TerrainSetting {
	public:
		static bool     smUseFrustumCull;
		static bool     smUseLodDebug;
		static bool     smUseRenderCameraDebug;
		static float	smNodeEvaluationC;
		static float	smTerrainMeterSize;
		static float	smTerrainHeightScale;
		static float	smMinLODNodeMeterSize;			// LOD0对应的地块大小为64.0f
		static float	smMaxLODNodeMeterSize;			// LOD4对应的地块大小为1024.0f
		static uint32_t	smMaxLOD;						// pow(2, 4) * 64 = 1024.0f，即 LOD0 1 2 3 4
		static uint32_t smPatchCountPerNodePerAxis;		// patch count
		static int32_t  smTerrainDataLoadedRange;		// 地形数据加载的范围
		static uint32_t smTerrainDataLoadedLimit;		// 一次加载多少个节点的数据

		static uint32_t smFarTerrainTextureAtlasTileCountPerAxis;	// 地形纹理图集每个轴的Tile个数
		static uint32_t smTerrainTiledSplatMapTileCountPerCache;	// TiledSplatMap最多保留多少Tile
		static int32_t  smTerrainTiledTextureDataLoadedRange;		// TiledTextureTile的加载范围
		static uint32_t smTerrainTiledTextureDataLoadedLimit;		// TiledTexture一次加载多少个Tile的数据

		static uint32_t smTerrainTiledGrassLandMapTileCountPerCache;
		static int32_t  smTerrainTiledGrassLandMapDataLoadedRange;
		static uint32_t smTerrainTiledGrassLandMapDataLoadedLimit;

		static uint32_t smTerrainFeedbackScale;				// TerrainFeedback相对于FinalOutput的缩放大小
		static uint32_t smRvtTileSizeNoPadding;				// 实时虚拟纹理中一个Tile的大小
		static uint32_t smRvtTilePaddingSize;				// 实时虚拟纹理中一个Tile的填充大小
		static uint32_t smRvtTileSizeWithPadding;
		static uint32_t smRvtTileCountPerAxisInAtlas;		// 实时虚拟纹理中纹理图集上每个轴的Tile个数
		static uint32_t smRvtAtlasTextureSize;				// 纹理图集的大小
		static float    smRvtRectRadius;					// 实时虚拟纹理的矩形半径(边长的一半)
		static uint32_t smRvtPixelSizePerMeter;				// 每米的像素值

		static bool     smRvtUsePageLevelDebug;						// 渲染时是否显示PageLevel
		static uint32_t smRvtMaxPageLevel;
		static uint32_t smRvtPageLevelBias;
		static uint32_t smRvtVirtualTextureSizeInBytesInPage0Level;	// VT大小InPage0Level
		static uint32_t smRvtTileCountPerAxisInPage0Level;			// 虚拟纹理页表(查找表)的大小
		static uint32_t smRvtVirtualTextureSizeInBytesInWorld;		// VT大小InWorld
		static uint32_t smRvtTileCountPerAxisInWorld;				// 世界中每个轴的Tile个数

		static float	 smWorldMeterSizePerTileInPage0Level;		// 纹理图集元素对应的世界大小
		static float    smWorldMeterSizePerPaddingInPage0Level;		// 图集元素的Padding对应的世界大小

		static uint32_t smTerrainFeedbackBufferElementCount;		// TerrainFeedbackBuffer中元素的个数

		static uint32_t smRvtDataLoadedLimit;						// 一次加载多少个虚拟纹理

		static float smRvtRealRectChangedViewDistance;				// Rvt更新一次对应摄像机移动的大小

		static uint32_t smWorldMeterSizePerTiledTexture;			// 对地形进行纹理平铺，多少米平铺一个地形纹理，此处的地形纹理是地形纹理数组中的纹理
	};

}