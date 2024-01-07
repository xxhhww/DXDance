#pragma once
#include <cstdint>

namespace Renderer {

	/*
	* ��������
	*/
	struct TerrainSetting {
	public:
		inline static float		nodeEvaluationC = 1.2f;
		inline static float		terrainMeterSize = 8192.0f;
		inline static float		terrainHeightScale = 1325.0f;
		inline static float		minLODNodeMeterSize = 64.0f;		// LOD0��Ӧ�ĵؿ��СΪ64.0f
		inline static float		maxLODNodeMeterSize = 1024.0f;		// LOD4��Ӧ�ĵؿ��СΪ1024.0f
		inline static uint32_t	maxLOD = 4.0f;						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
	};

}