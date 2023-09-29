#include "Game/Context.h"
#include "Core/ServiceLocator.h"

namespace Game {

	Context::Context(const std::string& name, HINSTANCE hInstance, int nCmdShow) {
		GHL::EnableDebugLayer();

		// 初始化Win32窗口
		Windows::WindowSetting winSetting{};
		winSetting.title = name;
		winSetting.width = 1920u;
		winSetting.height = 1080u;
		winSetting.fullscreen = false;
		winSetting.maximized = false;
		winSetting.hInstance = hInstance;
		winSetting.nCmdShow = nCmdShow;
		window = std::make_unique<Windows::Window>(winSetting);

		// 初始化输入设备管理
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// 初始化定时器
		clock = std::make_unique<Tool::Clock>();

		// 初始化内核渲染器，并将其注册为服务
		renderEngine = std::make_unique<Renderer::RenderEngine>(window->GetHWND(), winSetting.width, winSetting.height);

		// 注册服务
		Core::ServiceLocator::Provide(*renderEngine.get());
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*clock.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}

}