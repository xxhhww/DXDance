#pragma once
#include <string>

namespace Tool {
	class StrUtil {
	public:
		/*
		* string to wstring
		*/
		static std::wstring UTF8ToWString(const std::string& source);

		/*
		* 获得文件/路径的扩展名
		*/
		static std::string	GetFileExtension(const std::string& path);

		/*
		* 获得路径的最后一个层级
		*/
		static std::string	RemoveBasePath(const std::string& path);

		/*
		* 获得路径的目录层级(即去掉文件名)
		*/
		static std::string	GetBasePath(const std::string& path);
	};
}