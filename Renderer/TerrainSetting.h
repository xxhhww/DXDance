#pragma once
#include <cstdint>

namespace Renderer {

	/*
	* ��������
	*/
	struct TerrainSetting {
	public:
		static bool     smUseFrustumCull;
		static bool     smUseLodDebug;
		static float	smNodeEvaluationC;
		static float	smTerrainMeterSize;
		static float	smTerrainHeightScale;
		static float	smMinLODNodeMeterSize;			// LOD0��Ӧ�ĵؿ��СΪ64.0f
		static float	smMaxLODNodeMeterSize;			// LOD4��Ӧ�ĵؿ��СΪ1024.0f
		static uint32_t	smMaxLOD;						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
		static uint32_t smPatchCountPerNodePerAxis;		// patch count
		static int32_t  smTerrainDataLoadedRange;		// �������ݼ��صķ�Χ
		static uint32_t smTerrainDataLoadedLimit;		// һ�μ��ض��ٸ��ڵ������
	};

}