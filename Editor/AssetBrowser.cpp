#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include "UI/TextSelectable.h"
#include "UI/DDSource.h"
#include "UI/DDTarget.h"
#include "UI/Group.h"
#include "UI/Text.h"
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
	, mAssetPath(assetPath)
	, mCurrentPath(assetPath) {
		if (!std::filesystem::exists(mEnginePath)) {
			std::filesystem::create_directories(mEnginePath);
		}

		if (!std::filesystem::exists(mAssetPath)) {
			std::filesystem::create_directories(mAssetPath);
		}

		mVirtualFs = &CreateWidget<UI::Child>("VirtualFs", 0.2f);
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mEnginePath), false);
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mAssetPath), false);

		CreateWidget<UI::SameLine>();

		mAssetGrid = &CreateWidget<UI::Child>("Browser");
		mAssetGridColumns = &mAssetGrid->CreateWidget<UI::Columns>(0, mThumbnailSize + mGridPadding);
		BuildAssetGrid();
	}

	/*
	* 资产浏览器析构函数
	*/
	AssetBrowser::~AssetBrowser() {

	}

	/*
	* 递归函数，处理项目路径下的文件夹或者目录
	*/
	void AssetBrowser::BuildVirtualFs(UI::TreeNode* root, std::filesystem::directory_entry entry, bool isEngineItem) {
		std::string pathName = entry.path().string();
		std::string itemName = Tool::StrUtil::RemoveBasePath(pathName);
		bool isDirectory = entry.is_directory();
		auto& itemGroup = (root == nullptr ? mVirtualFs->CreateWidget<UI::Group>() : root->CreateWidget<UI::Group>());

		if (isDirectory) {
			auto& treeNode = itemGroup.CreateWidget<UI::TreeNode>(itemName);

			treeNode.openedEvent += [this, &treeNode, isEngineItem, pathName]() {
				treeNode.DeleteAllWidgets();
				for (auto& item : std::filesystem::directory_iterator(pathName)) {
					BuildVirtualFs(&treeNode, item, isEngineItem);
				}
			};

			treeNode.closedEvent += [&treeNode]() {
				treeNode.DeleteAllWidgets();
			};

			if (isEngineItem) {
				return;
			}


			treeNode.CreatePlugin<UI::DDSource<std::pair<std::string, UI::Group*>>>(
				"Folder",
				itemName,
				std::make_pair(pathName, &itemGroup));

			if (!root)
				treeNode.DeleteAllPlugins();

			treeNode.CreatePlugin<UI::DDTarget<std::pair<std::string, UI::Group*>>>("Folder").dataReceivedEvent += [&treeNode, pathName](const std::pair<std::string, UI::Group*>& data) {
				std::string prevPath = data.first;
				std::string itemName = Tool::StrUtil::RemoveBasePath(prevPath);
				std::string currPath = pathName + '\\' + itemName;

				if (prevPath == currPath) {
					return;
				}
				if (!std::filesystem::exists(prevPath)) {
					return;
				}
				if (!std::filesystem::exists(currPath)) {
					std::filesystem::create_directories(currPath);
					std::filesystem::copy(prevPath, currPath, std::filesystem::copy_options::recursive);
					std::filesystem::remove_all(prevPath);
					data.second->Destory();
					treeNode.openedEvent.Invoke();
				}
			};

			treeNode.CreatePlugin<UI::DDTarget<std::pair<std::string, UI::Group*>>>("File").dataReceivedEvent += [&treeNode, pathName](const std::pair<std::string, UI::Group*>& data) {
				std::string prevPath = data.first;
				std::string itemName = Tool::StrUtil::RemoveBasePath(prevPath);
				std::string currPath = pathName + '\\' + itemName;

				if (prevPath == currPath) {
					return;
				}
				if (std::filesystem::exists(currPath) || !std::filesystem::exists(prevPath)) {
					return;
				}

				std::filesystem::copy_file(prevPath, currPath);
				std::filesystem::remove(prevPath);
				treeNode.openedEvent.Invoke();

				data.second->Destory();
			};
		}
		else {
			auto& selectableText = itemGroup.CreateWidget<UI::TextSelectable>(itemName);

			if (isEngineItem) {
				return;
			}

			auto& ddSource = selectableText.CreatePlugin<UI::DDSource<std::pair<std::string, UI::Group*>>>(
				"File",
				itemName,
				std::make_pair(pathName, &itemGroup));
		}
	}

	/*
	* 绘制资产网格
	*/
	void AssetBrowser::BuildAssetGrid() {
		std::filesystem::directory_entry directory(mCurrentPath);
		for (auto& item : std::filesystem::directory_iterator(directory)) {
			auto& group = mAssetGridColumns->CreateWidget<UI::Group>();
			group.CreateWidget<UI::Button>(item.path().filename().string().c_str(), Math::Vector2{ mThumbnailSize, mThumbnailSize });
			group.CreateWidget<UI::Text>(item.path().filename().string().c_str());
			if (item.is_directory()) {

			}
			else {

			}
		}
	}
}