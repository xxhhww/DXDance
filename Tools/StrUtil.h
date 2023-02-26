#pragma once
#include <string>

namespace Tool {
	enum class FileType {
		TEXTURE	,
		MODEL	,
		AUDIO	,
		SHADER	,
		MATERIAL,
		UNSUPPORT,
	};

	class StrUtil {
	public:
		/*
		* string to wstring
		*/
		static std::wstring UTF8ToWString(const std::string& source);

		/*
		* wstring to string
		*/
		static std::string	WStringToUTF8(const std::wstring& source);

		/*
		* ȥ���ļ���׺
		*/
		static std::string	RemoveExtension(const std::string& path);

		/*
		* ����ļ�/·������չ��
		*/
		static std::string	GetFileExtension(const std::string& path);

		/*
		* ����ļ�����
		*/
		static FileType		GetFileType(const std::string& path);

		/*
		* ���·�������һ���㼶
		*/
		static std::string	RemoveBasePath(const std::string& path);

		/*
		* ���·����Ŀ¼�㼶(��ȥ���ļ���)
		*/
		static std::string	GetBasePath(const std::string& path);

		/*
		* �ַ���ǰ׺ƥ��
		*/
		static bool			StartWith(const std::string& path, const std::string& prefix);

		/*
		* ����·��������ʽ
		*/
		static std::string	MakeWindowsStyle(const std::string& path);
	};
}