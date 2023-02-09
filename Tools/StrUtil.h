#pragma once
#include <string>

namespace Tool {
	class StrUtil {
	public:
		static std::wstring ToWString(const std::string& source);
	};
}