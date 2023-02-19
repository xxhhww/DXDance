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
		* ����ļ�/·������չ��
		*/
		static std::string	GetFileExtension(const std::string& path);

		/*
		* ���·�������һ���㼶
		*/
		static std::string	RemoveBasePath(const std::string& path);
	};
}