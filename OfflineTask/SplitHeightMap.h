#pragma once
#include <string>

namespace OfflineTask {

	/*
	* �Ը߶�ͼ���е��úͷָ����
	*/
	class SplitHeightMap {
	public:
		void Run(const std::string& name, float tileCountPerAxis = smTerrainTileCountPerAxis);

	private:
		inline static float smWorldMeterSize{ 8192.0f };		// ������δ�С
		inline static float smTerrainTileMeterSize{ 256.0f };	// ���ηֿ��С
		inline static float smTerrainTileCountPerAxis{ smWorldMeterSize / smTerrainTileMeterSize };	// ÿ����ĵ��ηֿ����
	};

}