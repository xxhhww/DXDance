#pragma once
#include <string>
#include <memory>
#include "Windows/Window.h"
#include "Windows/InputManger.h"
#include "UI/UIManger.h"
#include "Tools/Clock.h"

namespace App {
	class Context {
	public:
		/*
		* ��ʼ����ע����ķ���
		*/
		Context(const std::string& projPath, const std::string& projName);

		/*
		* �Ƴ����������ķ���
		*/
		~Context();
	public:
		const std::string projectPath;
		const std::string projectName;
		const std::string projectEnginePath;
		const std::string projectAssetPath;

		std::unique_ptr<Windows::Window>		window;
		std::unique_ptr<Windows::InputManger>	inputManger;
		std::unique_ptr<UI::UIManger>			uiManger;		// UIManger�Ļ�����Editor���������
		std::unique_ptr<Tool::Clock>			clock;			// ��ʱ��
	};
}