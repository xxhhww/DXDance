#include "Game/Context.h"
#include "Core/ServiceLocator.h"

namespace Game {

	Context::Context(const std::string& name, HINSTANCE hInstance, int nCmdShow) {
		GHL::EnableDebugLayer();

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

		// ��ʼ���ں���Ⱦ����������ע��Ϊ����
		renderEngine = std::make_unique<Renderer::RenderEngine>(window->GetHWND(), winSetting.width, winSetting.height);

		// ע�����
		Core::ServiceLocator::Provide(*renderEngine.get());
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*clock.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}

}