#include "StrUtil.h"
#include <Windows.h>
#include <codecvt>

namespace Tool {
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
        int len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), 0, 0, 0, 0);
        char* buffer = new char[len + 1];
        WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), buffer, len, 0, 0);
        buffer[len] = '\0';
        std::string result(buffer, len);
        delete[] buffer;
        return result;
    }

    std::string	StrUtil::RemoveExtension(const std::string& path) {
        size_t pos{ 0u };
        if ((pos = path.rfind('.')) != std::string::npos) {
            return path.substr(0, pos);
        }

        return path;
    }

    std::string	StrUtil::GetFileExtension(const std::string& path) {
        size_t pos = path.find_last_of(".");
        return path.substr(pos + 1);
    }

    FileType StrUtil::GetFileType(const std::string& path) {
        std::string extension = StrUtil::GetFileExtension(path);

        if (extension == "png" || extension == "jpg" || extension == "tga" || extension == "dds") {
            return FileType::TEXTURE;
        }
        else if (extension == "pmx" || extension == "fbx" || extension == "obj") {
            return FileType::MODEL;
        }
        else if (extension == "wav" || extension == "mp3") {
            return FileType::AUDIO;
        }
        else if (extension == "scene") {
            return FileType::SCECNE;
        }
        else if (extension == "shader") {
            return FileType::SHADER;
        }
        else if (extension == "mat") {
            return FileType::MATERIAL;
        }

        return FileType::UNSUPPORT;
    }

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

    std::string StrUtil::GetFilename(const std::string& path) {
        return RemoveBasePath(RemoveExtension(path));
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