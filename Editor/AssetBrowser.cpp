#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include "UI/TextSelectable.h"
#include "UI/DDSource.h"
#include "UI/DDTarget.h"
#include "UI/Group.h"
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
		mVirtualFs->CreateWidget<UI::Separator>();
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mEnginePath), false);
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mAssetPath), false);

		CreateWidget<UI::SameLine>();

		mAssetGrid = &CreateWidget<UI::Child>("Browser");
		mCurrPathText = &mAssetGrid->CreateWidget<UI::Text>(mCurrentPath);
		mAssetGrid->CreateWidget<UI::Separator>();
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
		auto& itemGroup = (root == nullptr ? mVirtualFs->CreateWidgetDelay<UI::Group>() : root->CreateWidgetDelay<UI::Group>());

		if (isDirectory) {
			auto& treeNode = itemGroup.CreateWidgetDelay<UI::TreeNode>(itemName);

			treeNode.openedEvent += [this, &treeNode, isEngineItem, pathName]() {
				treeNode.DestoryAllWidgets();
				for (auto& item : std::filesystem::directory_iterator(pathName)) {
					BuildVirtualFs(&treeNode, item, isEngineItem);
				}

				mCurrentPath = pathName;
				BuildAssetGrid();
			};

			treeNode.clickedEvent += [this, &treeNode, pathName]() {
				if (treeNode.opened) {
					mCurrentPath = pathName;
					BuildAssetGrid();
				}
			};

			treeNode.closedEvent += [&treeNode]() {
				treeNode.DestoryAllWidgets();
			};

			treeNode.CreatePlugin<UI::DDSource<std::pair<std::string, UI::Group*>>>(
				"Folder",
				itemName,
				std::make_pair(pathName, &itemGroup));

			if (!root)
				treeNode.DeleteAllPlugins();

			// 引擎文件和文件夹无法成为拖动的目标
			if (isEngineItem) {
				return;
			}

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

					if (data.second != nullptr) {
						data.second->Destory();
					}
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

				if (data.second != nullptr) {
					data.second->Destory();
				}
			};
		}
		else {
			auto& selectableText = itemGroup.CreateWidgetDelay<UI::TextSelectable>(itemName);

			selectableText.clickedEvent += [this, pathName]() {
				mCurrentPath = Tool::StrUtil::GetBasePath(pathName);
				BuildAssetGrid();
			};

			// 引擎文件和普通文件均可拖动
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
		mCurrPathText->data = mCurrentPath;

		mAssetGridColumns->DestoryAllWidgets();

		std::filesystem::directory_entry directory(mCurrentPath);
		for (auto& item : std::filesystem::directory_iterator(directory)) {
			std::string pathName = item.path().string();
			std::string itemName = Tool::StrUtil::RemoveBasePath(pathName);

			auto& group = mAssetGridColumns->CreateWidgetDelay<UI::Group>();
			auto& button = group.CreateWidgetDelay<UI::Button>(itemName, Math::Vector2{ mThumbnailSize, mThumbnailSize });
			group.CreateWidgetDelay<UI::Text>(itemName);
			if (item.is_directory()) {
				button.doubleClickedEvent += [this, pathName]() {
					mCurrentPath = pathName;
					BuildAssetGrid();
				};
			}
			else {
				button.CreatePlugin<UI::DDSource<std::pair<std::string, UI::Group*>>>(
					"File",
					itemName,
					std::make_pair(pathName, nullptr));
			}
		}
	}
}