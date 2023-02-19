#include "StrUtil.h"
#include <Windows.h>

namespace Tool {
    /*
    * string to wstring
    */
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

    /*
    * 获得文件/路径的扩展名
    */
    std::string	StrUtil::GetFileExtension(const std::string& path) {
        size_t pos = path.find_last_of(".");
        return path.substr(pos + 1);
    }

    /*
    * 获得路径的最后一个层级
    */
    std::string	StrUtil::RemoveBasePath(const std::string& path) {
        size_t pos{ 0u };

        if ((pos = path.rfind('/')) != std::string::npos) {
            return path.substr(pos + 1);
        }
        else if (pos = path.rfind('\\') != std::string::npos) {
            return path.substr(pos + 1);
        }

        return path;
    }
}