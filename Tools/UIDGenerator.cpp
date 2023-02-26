#include "UIDGenerator.h"
#include <chrono>
#include <random>

namespace Tool {
	int64_t UIDGenerator::Get() {
		// 获取操作系统当前时间点（精确到微秒）
		std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMicro 
			= std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());

		// (微秒精度的)时间点 => (微秒精度的)时间戳
		time_t totalMicroSeconds = tpMicro.time_since_epoch().count();

		return totalMicroSeconds;
	}
}