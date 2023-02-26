#include "StrUtil.h"
#include <Windows.h>
#include <codecvt>

namespace Tool {
    /*
    * string to wstring
    */
	std::wstring StrUtil::UTF8ToWString(const std::string& str) {
        int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
        if (len < 0)return L"";
        wchar_t* buffer = new wchar_t[len + 1];
        if (buffer == NULL)return L"";
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
        buffer[len] = '\0';
        std::wstring result(buffer, len);
        delete[] buffer;
        return result;
	}

    std::string StrUtil::WStringToUTF8(const std::wstring& str) {
        int len;
        int slength = (int)str.length() + 1;
        len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), slength, 0, 0, 0, 0);
        char* buf = new char[len];
        WideCharToMultiByte(CP_ACP, 0, str.c_str(), slength, buf, len, 0, 0);
        std::string r(buf, len);
        delete[] buf;
        return r;
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
        else if ((pos = path.rfind('\\')) != std::string::npos) {
            return path.substr(pos + 1);
        }

        return path;
    }

    std::string	StrUtil::GetBasePath(const std::string& path) {
        size_t pos{ 0u };
        if ((pos = path.rfind('/')) != std::string::npos) {
            return path.substr(0, pos);
        }
        else if ((pos = path.rfind('\\')) != std::string::npos) {
            return path.substr(0, pos);
        }

        return path;
    }

    bool StrUtil::StartWith(const std::string& path, const std::string& prefix) {
        return path.size() >= prefix.size() && path.compare(0, prefix.size(), prefix) == 0;
    }

    std::string StrUtil::MakeWindowsStyle(const std::string& path) {
        std::string result;
        result.resize(path.size());

        for (size_t i = 0; i < path.size(); ++i) {
            result[i] = path[i] == '/' ? '\\' : path[i];
        }

        return result;
    }
}