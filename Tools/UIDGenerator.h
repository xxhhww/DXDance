#pragma once
#include <stdint.h>

namespace Tool {
	class UIDGenerator {
	public:
		/*
		* 使用当前系统的微秒级时间来创建UID
		*/
		static int64_t Get();
	};
}