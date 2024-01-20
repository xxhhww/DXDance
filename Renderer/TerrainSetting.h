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
	};

}