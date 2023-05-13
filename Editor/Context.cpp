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

		// 初始化资产管理器
		assetPathDataBase = std::make_unique<Core::AssetPathDataBase>("Undefined");
		sceneManger = std::make_unique<Core::SceneManger>(projectAssetPath, projectEnginePath, assetPathDataBase.get());

		// 初始化内核引擎，并将其注册为服务
		renderEngine = std::make_unique<Renderer::RenderEngine>(nullptr, 1920u, 1080u);
		Core::ServiceLocator::Provide(*renderEngine.get());

		// 初始化编辑器资源管理器
		editorAssetManger = std::make_unique<Core::EditorAssetManger>("E:/MyProject/DXDance/Resources");

		// 注册服务
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*uiManger.get());
		Core::ServiceLocator::Provide(*clock.get());
		Core::ServiceLocator::Provide(*assetPathDataBase.get());
		Core::ServiceLocator::Provide(*sceneManger.get());
		Core::ServiceLocator::Provide(*editorAssetManger.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}