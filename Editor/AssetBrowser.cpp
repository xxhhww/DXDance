#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include "Tools/StrUtil.h"

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

		mVirtualFolder = &CreateWidget<UI::Child>("Folder", 0.2f);
		auto& importButton = mVirtualFolder->CreateWidget<UI::Button>("Import");
		mVirtualFolder->CreateWidget<UI::Separator>();

		CreateWidget<UI::SameLine>();

		mAssetGrid = &CreateWidget<UI::Child>("Grid");
		mAssetGrid->CreateWidget<UI::Separator>();

		ConsiderItem(nullptr, std::filesystem::directory_entry(mEnginePath), true);
		ConsiderItem(nullptr, std::filesystem::directory_entry(mAssetPath), false);
	}

	/*
	* 资产浏览器析构函数
	*/
	AssetBrowser::~AssetBrowser() {

	}

	/*
	* 递归函数，处理项目路径下的文件夹或者目录
	*/
	void AssetBrowser::ConsiderItem(UI::TreeNode* root, std::filesystem::directory_entry entry, bool isEngineItem) {
		std::string pathName = entry.path().string();
		std::string itemName = Tool::StrUtil::RemoveBasePath(pathName);
		bool isDirectory = entry.is_directory();
		auto& itemGroup = (root == nullptr ? mVirtualFolder->CreateWidget<UI::Group>() : root->CreateWidget<UI::Group>());

		if (isDirectory) {
			auto& treeNode = itemGroup.CreateWidget<UI::TreeNode>(itemName);

			treeNode.openedEvent += [this, &treeNode, &entry, &isEngineItem]() {
				treeNode.DeleteAllWidgets();
				for (auto& item : std::filesystem::directory_iterator("E:\\DXDanceProj\\Engine")) {
					ConsiderItem(&treeNode, item, isEngineItem);
				}
			};

			treeNode.closedEvent += [&treeNode]() {
				treeNode.DeleteAllWidgets();
			};
			
		}


	}
}