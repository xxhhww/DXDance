#include "Context.h"
#include "Core/ServiceLocator.h"

namespace App {
	/*
	* ��ʼ����ע����ķ���
	*/
	Context::Context(const std::string& projPath, const std::string& projName)
	: projectPath(projectPath)
	, projectName(projectName)
	, projectAssetPath(projectPath + "\\Assets")
	, projectShaderPath(projectAssetPath + "\\Shaders")
	, projectMaterialPath(projectAssetPath + "\\Materials") {
		// ��ʼ��Win32����
		Windows::WindowSetting winSetting{};
		winSetting.fullscreen = false;
		winSetting.visible = true;
		winSetting.maximized = true;
		winSetting.resizable = true;
		winSetting.title = projName;
		window = std::make_unique<Windows::Window>();

		// ��ʼ�������豸����
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// ��ʼ��UI����
		uiManger = std::make_unique<UI::UIManger>(window.get(), UI::UIStyle::DARK);

		// ע�����
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*uiManger.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}