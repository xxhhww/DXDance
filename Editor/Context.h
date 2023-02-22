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
		* 初始化并注册核心服务
		*/
		Context(const std::string& projPath, const std::string& projName);

		/*
		* 移除并析构核心服务
		*/
		~Context();
	public:
		const std::string projectPath;
		const std::string projectName;
		const std::string projectEnginePath;
		const std::string projectAssetPath;

		std::unique_ptr<Windows::Window>		window;
		std::unique_ptr<Windows::InputManger>	inputManger;
		std::unique_ptr<UI::UIManger>			uiManger;		// UIManger的画布由Editor类进行设置
		std::unique_ptr<Tool::Clock>			clock;			// 定时器
	};
}