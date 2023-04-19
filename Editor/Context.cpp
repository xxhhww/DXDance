#include "Context.h"
#include "Core/ServiceLocator.h"

namespace App {
	/*
	* ��ʼ����ע����ķ���
	*/
	Context::Context(const std::string& projPath, const std::string& projName)
	: projectPath(projPath)
	, projectName(projName)
	, projectEnginePath(projPath + "\\Engine")
	, projectAssetPath(projPath + "\\Assets") {
		// ��ʼ��Win32����
		Windows::WindowSetting winSetting{};
		winSetting.title = projName;
		window = std::make_unique<Windows::Window>();

		// ��ʼ�������豸����
		inputManger = std::make_unique<Windows::InputManger>(window.get());

		// ��ʼ��UI����
		uiManger = std::make_unique<UI::UIManger>(window.get(), UI::UIStyle::DARK);

		// ��ʼ����ʱ��
		clock = std::make_unique<Tool::Clock>();

		// ��ʼ���ʲ�������
		assetPathDataBase = std::make_unique<Core::AssetPathDataBase>("Undefined");
		sceneManger = std::make_unique<Core::SceneManger>(projectAssetPath, projectEnginePath, assetPathDataBase.get());

		// ע�����
		Core::ServiceLocator::Provide(*window.get());
		Core::ServiceLocator::Provide(*inputManger.get());
		Core::ServiceLocator::Provide(*uiManger.get());
		Core::ServiceLocator::Provide(*clock.get());
		Core::ServiceLocator::Provide(*assetPathDataBase.get());
		Core::ServiceLocator::Provide(*sceneManger.get());
	}

	Context::~Context() {
		Core::ServiceLocator::RemoveAllServices();
	}
}