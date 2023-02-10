#pragma once
#include <string>

namespace Tool {
	class StrUtil {
	public:
		static std::wstring UTF8ToWString(const std::string& source);
	};
}