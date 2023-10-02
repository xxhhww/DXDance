#include "Game/Context.h"
#include "Game/CustomMemoryHook.h"
#include "Core/ServiceLocator.h"

#include "ECS/Entity.h"

namespace Game {

	Context::Context(const std::string& name, HINSTANCE hInstance, int nCmdShow) {
		GHL::EnableDebugLayer();
		RegisterCustomMemoryHook();

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

		// 初始化任务系统
		jobSystem = std::make_unique<JPH::JobSystemThreadPool>(maxJobs, maxBarriers, std::thread::hardware_concurrency() - 4);

		// 为ECS设置任务系统
		ECS::Entity::SetJobSystem(jobSystem.get());

		// 初始化物理系统
		physicsSystem = std::make_unique<Physics::PhysicsSystem>(jobSystem.get());

		// 初始化渲染器
		renderEngine = std::make_unique<Renderer::RenderEngine>(window->GetHWND(), winSetting.width, winSetting.height);

		// 初始化业务系统
		systemManger = std::make_unique<Game::SystemManger>();

		// 初始化游戏资产
		assetManger = std::make_unique<Game::AssetManger>();

		// 初始化全局数据
		globalData = std::make_unique<Game::GlobalData>();

		// 注册全局服务
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*clock.get());
		Core::ServiceLocator::Provide(*jobSystem.get());
		Core::ServiceLocator::Provide(*physicsSystem.get());
		Core::ServiceLocator::Provide(*renderEngine.get());
		Core::ServiceLocator::Provide(*systemManger.get());
		Core::ServiceLocator::Provide(*assetManger.get());
		Core::ServiceLocator::Provide(*globalData.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}

}