#include "Context.h"
#include "Core/ServiceLocator.h"

namespace App {
	/*
	* 初始化并注册核心服务
	*/
	Context::Context(const std::string& projPath, const std::string& projName)
	: projectPath(projPath)
	, projectName(projName)
	, projectEnginePath(projPath + "\\Engine")
	, projectAssetPath(projPath + "\\Assets") {
		// 初始化Win32窗口
		Windows::WindowSetting winSetting{};
		winSetting.title = projName;
		window = std::make_unique<Windows::Window>();

		// 初始化输入设备管理
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// 初始化UI管理
		uiManger = std::make_unique<UI::UIManger>(window.get(), UI::UIStyle::DARK);

		// 初始化定时器
		clock = std::make_unique<Tool::Clock>();

		// 注册服务
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*uiManger.get());
		Core::ServiceLocator::Provide(*clock.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}