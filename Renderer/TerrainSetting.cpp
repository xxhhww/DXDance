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
	uint32_t TerrainSetting::smTerrainFeedbackScale = 4;		// Feedback相对于FinalOutput的缩放大小
}