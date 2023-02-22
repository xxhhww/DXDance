#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include <filesystem>

namespace App {
	AssetBrowser::AssetBrowser(
		const std::string& title,
		const std::string& enginePath,
		const std::string& assetPath,
		bool opened,
		const UI::PanelWindowSettings& panelSetting
	)
	: PanelWindow(title, opened, panelSetting)
	, mEnginePath(enginePath)
	, mAssetPath(assetPath) {
		if (!std::filesystem::exists(mEnginePath)) {
			std::filesystem::create_directories(mEnginePath);
		}

		if (!std::filesystem::exists(mAssetPath)) {
			std::filesystem::create_directories(mAssetPath);
		}

		mColumns = &CreateWidget<UI::Columns<2>>();
		mColumns->widths[0] = 300.0f;

		mVirtualFolder = &(mColumns->CreateWidget<UI::Group>());
		auto& importButton = mVirtualFolder->CreateWidget<UI::Button>("Import");
		mVirtualFolder->CreateWidget<UI::Separator>();

		mAssetList = &(mColumns->CreateWidget<UI::Group>());
		mVirtualFolder->CreateWidget<UI::Separator>();
	}

	/*
	* 资产浏览器析构函数
	*/
	AssetBrowser::~AssetBrowser() {

	}
}