#pragma once
#include <string>

namespace OfflineTask {

	/*
	* 对高度图进行倒置和分割操作
	*/
	class SplitHeightMap {
	public:
		void Run(const std::string& name, float tileCountPerAxis = smTerrainTileCountPerAxis);

	private:
		inline static float smWorldMeterSize{ 8192.0f };		// 世界地形大小
		inline static float smTerrainTileMeterSize{ 256.0f };	// 地形分块大小
		inline static float smTerrainTileCountPerAxis{ smWorldMeterSize / smTerrainTileMeterSize };	// 每个轴的地形分块个数
	};

}