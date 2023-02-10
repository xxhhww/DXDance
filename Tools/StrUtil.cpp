#include "StrUtil.h"
#include <Windows.h>

namespace Tool {
	std::wstring StrUtil::UTF8ToWString(const std::string& str) {
        std::wstring result;
        int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
        if (len < 0)return result;
        wchar_t* buffer = new wchar_t[len + 1];
        if (buffer == NULL)return result;
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
        buffer[len] = '\0';
        result.append(buffer);
        delete[] buffer;
        return result;
	}
}