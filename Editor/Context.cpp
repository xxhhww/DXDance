#include "Context.h"
#include "Core/ServiceLocator.h"

namespace Editor {
	Context::Context(const std::string& projPath, const std::string& projName)
	: projectPath(projectPath)
	, projectName(projectName)
	, projectAssetPath(projectPath + "\\Assets")
	, projectShaderPath(projectAssetPath + "\\Shaders")
	, projectMaterialPath(projectAssetPath + "\\Materials") {
		// 初始化并注册核心服务

		// 初始化Win32窗口
		Windows::WindowSetting winSetting{};
		winSetting.fullscreen = false;
		winSetting.visible = true;
		winSetting.maximized = true;
		winSetting.resizable = true;
		winSetting.title = projName;
		window = std::make_unique<Windows::Window>();

		// 初始化输入设备管理
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// 注册服务
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}