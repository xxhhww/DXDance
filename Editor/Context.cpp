#include "Context.h"
#include "Core/ServiceLocator.h"

namespace Editor {
	Context::Context(const std::string& projPath, const std::string& projName)
	: projectPath(projectPath)
	, projectName(projectName)
	, projectAssetPath(projectPath + "\\Assets")
	, projectShaderPath(projectAssetPath + "\\Shaders")
	, projectMaterialPath(projectAssetPath + "\\Materials") {
		// ��ʼ����ע����ķ���

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

		// ע�����
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}