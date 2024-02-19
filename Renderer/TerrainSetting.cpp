#include "Renderer/TerrainSetting.h"

namespace Renderer {

	bool     TerrainSetting::smUseFrustumCull = false;
	bool     TerrainSetting::smUseLodDebug = true;
	float	 TerrainSetting::smNodeEvaluationC = 1.2f;
	float	 TerrainSetting::smTerrainMeterSize = 8192.0f;
	float	 TerrainSetting::smTerrainHeightScale = 1325.0f;
	float	 TerrainSetting::smMinLODNodeMeterSize = 64.0f;		// LOD0对应的地块大小为64.0f
	float	 TerrainSetting::smMaxLODNodeMeterSize = 1024.0f;	// LOD4对应的地块大小为1024.0f
	uint32_t TerrainSetting::smMaxLOD = 4;						// pow(2, 4) * 64 = 1024.0f，即 LOD0 1 2 3 4
	uint32_t TerrainSetting::smPatchCountPerNodePerAxis = 8;	// patch count
	int32_t  TerrainSetting::smTerrainDataLoadedRange = 4;		// 地形数据加载范围(离摄像机)
	uint32_t TerrainSetting::smTerrainDataLoadedLimit = 16;		// 一次加载64个地形节点的数据

	uint32_t TerrainSetting::smFarTerrainTextureAtlasTileCountPerAxis = 25u;	// 地形纹理图集每个轴的Tile个数

	uint32_t TerrainSetting::smTerrainFeedbackScale = 4;		// TerrainFeedback相对于FinalOutput的缩放大小
	uint32_t TerrainSetting::smRvtTileSizeNoPadding = 256;		// 实时虚拟纹理中一个Tile的大小
	uint32_t TerrainSetting::smRvtTilePaddingSize = 4;			// 实时虚拟纹理中一个Tile的填充大小
	uint32_t TerrainSetting::smRvtTileCountPerAxisInAtlas = 15;	// 实时虚拟纹理中实际纹理上每个轴的Tile个数
	int32_t  TerrainSetting::smRvtRectRadius = 1024;			// 实时虚拟纹理的矩形半径(边长的一半)
	uint32_t TerrainSetting::smRvtPixelSizePerMeter = 256;		// 每米的像素值

	uint32_t TerrainSetting::smRvtMaxPageLevel = 12;			// log2(smRvtRectRadius * 2)
	uint32_t TerrainSetting::smRvtPageLevelBias = 0;
	uint32_t TerrainSetting::smRvtVirtualTextureSizeInBytesInPage0Level = smRvtRectRadius * 2 * smRvtPixelSizePerMeter;
	uint32_t TerrainSetting::smRvtTileCountPerAxisInPage0Level = (smRvtVirtualTextureSizeInBytesInPage0Level / smRvtTileSizeNoPadding);	// 虚拟纹理页表(查找表)的大小
	uint32_t TerrainSetting::smRvtVirtualTextureSizeInBytesInWorld = smTerrainMeterSize * smRvtPixelSizePerMeter;
	uint32_t TerrainSetting::smRvtTileCountPerAxisInWorld = (smRvtVirtualTextureSizeInBytesInWorld / smRvtTileSizeNoPadding);			// 世界中每个轴的Tile个数
}