#include "Renderer/TerrainSetting.h"

namespace Renderer {

	bool     TerrainSetting::smUseFrustumCull = false;
	bool     TerrainSetting::smUseLodDebug = true;
	float	 TerrainSetting::smNodeEvaluationC = 1.2f;
	float	 TerrainSetting::smTerrainMeterSize = 8192.0f;
	float	 TerrainSetting::smTerrainHeightScale = 1325.0f;
	float	 TerrainSetting::smMinLODNodeMeterSize = 64.0f;		// LOD0��Ӧ�ĵؿ��СΪ64.0f
	float	 TerrainSetting::smMaxLODNodeMeterSize = 1024.0f;	// LOD4��Ӧ�ĵؿ��СΪ1024.0f
	uint32_t TerrainSetting::smMaxLOD = 4;						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
	uint32_t TerrainSetting::smPatchCountPerNodePerAxis = 8;	// patch count
	int32_t  TerrainSetting::smTerrainDataLoadedRange = 4;		// �������ݼ��ط�Χ(�������)
	uint32_t TerrainSetting::smTerrainDataLoadedLimit = 16;		// һ�μ���64�����νڵ������
	uint32_t TerrainSetting::smTerrainFeedbackScale = 4;		// Feedback�����FinalOutput�����Ŵ�С
}