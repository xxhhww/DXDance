#include "Game/Context.h"
#include "Game/CustomMemoryHook.h"
#include "Core/ServiceLocator.h"

#include "ECS/Entity.h"

namespace Game {

	Context::Context(const std::string& name, HINSTANCE hInstance, int nCmdShow) {
		GHL::EnableDebugLayer();
		RegisterCustomMemoryHook();

		// ��ʼ��Win32����
		Windows::WindowSetting winSetting{};
		winSetting.title = name;
		winSetting.width = 1920u;
		winSetting.height = 1080u;
		winSetting.fullscreen = false;
		winSetting.maximized = false;
		winSetting.hInstance = hInstance;
		winSetting.nCmdShow = nCmdShow;
		window = std::make_unique<Windows::Window>(winSetting);

		// ��ʼ�������豸����
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// ��ʼ����ʱ��
		clock = std::make_unique<Tool::Clock>();

		// ��ʼ������ϵͳ
		jobSystem = std::make_unique<JPH::JobSystemThreadPool>(maxJobs, maxBarriers, std::thread::hardware_concurrency() - 4);

		// ΪECS��������ϵͳ
		ECS::Entity::SetJobSystem(jobSystem.get());

		// ��ʼ������ϵͳ
		physicsSystem = std::make_unique<Physics::PhysicsSystem>(jobSystem.get());

		// ��ʼ����Ⱦ��
		renderEngine = std::make_unique<Renderer::RenderEngine>(window->GetHWND(), winSetting.width, winSetting.height);

		// ��ʼ��ҵ��ϵͳ
		systemManger = std::make_unique<Game::SystemManger>();

		// ��ʼ����Ϸ�ʲ�
		assetManger = std::make_unique<Game::AssetManger>();

		// ��ʼ��ȫ������
		globalData = std::make_unique<Game::GlobalData>();

		// ע��ȫ�ַ���
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