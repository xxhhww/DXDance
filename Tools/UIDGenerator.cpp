#include "UIDGenerator.h"
#include <chrono>
#include <random>

namespace Tool {
	int64_t UIDGenerator::Get() {
		// ��ȡ����ϵͳ��ǰʱ��㣨��ȷ��΢�룩
		std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMicro 
			= std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());

		// (΢�뾫�ȵ�)ʱ��� => (΢�뾫�ȵ�)ʱ���
		time_t totalMicroSeconds = tpMicro.time_since_epoch().count();

		return totalMicroSeconds;
	}
}