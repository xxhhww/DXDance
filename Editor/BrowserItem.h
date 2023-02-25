#pragma once
#include "UI/IWidget.h"
#include "Tools/Event.h"

namespace App {
	/*
	* �������Ŀ(���ļ�����Ŀ���ļ���Ŀ�Ļ���)
	*/
	class BrowserItem : public UI::IWidget {
	public:
		/*
		* ���캯��
		*/
		BrowserItem(const std::string& name, const std::string& path, bool isDirectory);
	public:
		std::string name;			// ��Ŀ����
		std::string path;			// ��Ŀ·��
		bool		isDirectory;	// �Ƿ�ΪĿ¼
	};
}