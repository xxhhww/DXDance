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


		static uint32_t smTerrainFeedbackScale;				// TerrainFeedback相对于FinalOutput的缩放大小
		static uint32_t smRvtTileSizeNoPadding;				// 实时虚拟纹理中一个Tile的大小
		static uint32_t smRvtTilePaddingSize;				// 实时虚拟纹理中一个Tile的填充大小
		static uint32_t smRvtTileCountPerAxisInAtlas;		// 实时虚拟纹理中纹理图集上每个轴的Tile个数
		static int32_t  smRvtRectRadius;					// 实时虚拟纹理的矩形半径(边长的一半)
		static uint32_t smRvtPixelSizePerMeter;				// 每米的像素值

		static uint32_t smRvtMaxPageLevel;
		static uint32_t smRvtPageLevelBias;
		static uint32_t smRvtVirtualTextureSizeInBytesInPage0Level;	// VT大小InPage0Level
		static uint32_t smRvtTileCountPerAxisInPage0Level;			// 虚拟纹理页表(查找表)的大小
		static uint32_t smRvtVirtualTextureSizeInBytesInWorld;		// VT大小InWorld
		static uint32_t smRvtTileCountPerAxisInWorld;				// 世界中每个轴的Tile个数
	};

}