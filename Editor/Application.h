#pragma once
#include "Editor.h"
#include "Context.h"
#include <string>

namespace App {
	class Application {
	public:
		/*
		* ��ʼ��������������༭��
		*/
		Application(const std::string& projPath = "E:\\DXDanceProj", const std::string& projName = "DXDacneTest");

		/*
		* Ĭ����������
		*/
		~Application() = default;

		/*
		* ����Application
		*/
		int Run();
	private:
		Context mContext;
		Editor mEditor;
	};
}