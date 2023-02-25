#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include "UI/TextSelectable.h"
#include "UI/DDSource.h"
#include "UI/DDTarget.h"
#include "UI/Group.h"
#include "Tools/StrUtil.h"
#include "FileItem.h"
#include "FolderItemContextualMenu.h"
#include "FileItemContextualMenu.h"


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

		mVirtualFs = &CreateWidget<UI::Child>("VirtualFs", 0.2f);
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mEnginePath), true);
		BuildVirtualFs(nullptr, std::filesystem::directory_entry(mAssetPath), false);

		CreateWidget<UI::SameLine>();

		mFolderItemView = &CreateWidget<UI::Child>("Browser");
		mViewPathText = &mFolderItemView->CreateWidget<UI::Text>(mAssetFolderItem->path);
		mFolderItemView->CreateWidget<UI::Separator>();
		mViewColumns = &mFolderItemView->CreateWidget<UI::Columns>(0, mThumbnailSize + mGridPadding);
		BuildVirtualFsView(mAssetFolderItem);
	}

	/*
	* �ʲ��������������
	*/
	AssetBrowser::~AssetBrowser() {

	}

	/*
	* �ݹ麯����������Ŀ·���µ��ļ��л���Ŀ¼
	*/
	void AssetBrowser::BuildVirtualFs(FolderItem* root, std::filesystem::directory_entry entry, bool isEngineItem) {
		// ��ȡ��Ŀ·�������ơ��Ƿ�Ϊ�ļ���
		std::string path = entry.path().string();
		std::string name = Tool::StrUtil::RemoveBasePath(path);
		bool isFolder = entry.is_directory();

		if (isFolder) {
			auto& treeNode = (root == nullptr) ? mVirtualFs->CreateWidget<FolderItem>(name, path) : root->CreateBrowserItem<FolderItem>(name, path);
			if (root == nullptr) {
				mEngineFolderItem = isEngineItem ? &treeNode : nullptr;
				mAssetFolderItem  = isEngineItem ? nullptr : &treeNode;
			}

			treeNode.CreatePlugin<UI::DDSource<std::pair<std::string, BrowserItem*>>>(
				"Folder",
				name,
				std::make_pair(path, &treeNode));

			// ��Ŀ¼������Ϊ��ק��Դ
			if (root == nullptr) {
				treeNode.DeleteAllPlugins();
			}

			auto& contextualMenu = treeNode.CreatePlugin<FolderItemContextualMenu>(name, path, isEngineItem);
			contextualMenu.BuildPopupContextItem();

			contextualMenu.itemAddedEvent += [this, &treeNode](const std::string& path) {
				BuildVirtualFs(&treeNode, std::filesystem::directory_entry(path), false);
			};

			// �����ļ����ļ����޷���Ϊ�϶���Ŀ��
			if (!isEngineItem) {
				treeNode.CreatePlugin<UI::DDTarget<std::pair<std::string, BrowserItem*>>>("Folder").dataReceivedEvent += [this, &treeNode, path](const std::pair<std::string, BrowserItem*>& data) {
					std::string prevPath = data.first;
					std::string name = Tool::StrUtil::RemoveBasePath(prevPath);
					std::string newPath = path + '\\' + name;

					bool engineItem = IsEngineItem(prevPath);

					if (prevPath == newPath) {
						return;
					}
					if (!std::filesystem::exists(prevPath)) {
						return;
					}
					if (!std::filesystem::exists(newPath)) {
						std::filesystem::create_directories(newPath);
						std::filesystem::copy(prevPath, newPath, std::filesystem::copy_options::recursive);
						if (!engineItem) {
							std::filesystem::remove_all(prevPath);
						}
						BuildVirtualFs(&treeNode, std::filesystem::directory_entry(newPath), false);

						if (data.second != nullptr && !engineItem) {
							data.second->Destory();
						}
					}
				};

				treeNode.CreatePlugin<UI::DDTarget<std::pair<std::string, BrowserItem*>>>("File").dataReceivedEvent += [this, &treeNode, path](const std::pair<std::string, BrowserItem*>& data) {
					std::string prevPath = data.first;
					std::string name = Tool::StrUtil::RemoveBasePath(prevPath);
					std::string newPath = path + '\\' + name;

					bool engineItem = IsEngineItem(prevPath);

					if (prevPath == newPath) {
						return;
					}
					if (std::filesystem::exists(newPath) || !std::filesystem::exists(prevPath)) {
						return;
					}

					std::filesystem::copy_file(prevPath, newPath);
					if (!engineItem) {
						std::filesystem::remove(prevPath);
					}
					BuildVirtualFs(&treeNode, std::filesystem::directory_entry(newPath), false);

					if (data.second != nullptr && !engineItem) {
						data.second->Destory();
					}
				};
			}

			for (auto& item : std::filesystem::directory_iterator(entry)) {
				BuildVirtualFs(&treeNode, item, isEngineItem);
			}
		}
		else {
			assert(root != nullptr);
			auto& leafNode = root->CreateBrowserItem<FileItem>(name, path);

			// �����ļ�����ͨ�ļ������϶�
			leafNode.CreatePlugin<UI::DDSource<std::pair<std::string, BrowserItem*>>>(
				"File",
				name,
				std::make_pair(path, &leafNode));
		}
		/*
		std::string pathName = entry.path().string();
		std::string itemName = Tool::StrUtil::RemoveBasePath(pathName);
		bool isDirectory = entry.is_directory();
		auto& itemGroup = (root == nullptr ? mVirtualFs->CreateWidgetDelay<UI::Group>() : root->CreateWidgetDelay<UI::Group>());

		if (isDirectory) {
			auto& treeNode = itemGroup.CreateWidget<UI::TreeNode>(itemName);

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

			// �����ļ����ļ����޷���Ϊ�϶���Ŀ��
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
			auto& selectableText = itemGroup.CreateWidget<UI::TextSelectable>(itemName);

			selectableText.clickedEvent += [this, pathName]() {
				mCurrentPath = Tool::StrUtil::GetBasePath(pathName);
				BuildAssetGrid();
			};

			// �����ļ�����ͨ�ļ������϶�
			auto& ddSource = selectableText.CreatePlugin<UI::DDSource<std::pair<std::string, UI::Group*>>>(
				"File",
				itemName,
				std::make_pair(pathName, &itemGroup));
		}
		*/
	}

	void AssetBrowser::BuildVirtualFsView(FolderItem* node) {
		mCurrFolderItem = node;
		mViewPathText->data = mCurrFolderItem->path;

		mViewColumns->DestoryAllWidgets();

		auto& browserItems = mCurrFolderItem->GetBrowserItems();
		for (auto& pair : browserItems) {
			auto& item = pair.second;

			auto& group = mViewColumns->CreateWidgetDelay<UI::Group>();
			auto& button = group.CreateWidget<UI::Button>(item->name, Math::Vector2{ mThumbnailSize, mThumbnailSize });
			group.CreateWidget<UI::Text>(item->name);

			if (item->isDirectory) {
				button.doubleClickedEvent += [this, &item]() {
					BuildVirtualFsView(reinterpret_cast<FolderItem*>(item.get()));
				};
			}
			else {
				button.CreatePlugin<UI::DDSource<std::pair<std::string, BrowserItem*>>>(
					"File",
					item->name,
					std::make_pair(item->path, item.get()));
			}
		}
	}

	bool AssetBrowser::IsEngineItem(const std::string& path) {
		return Tool::StrUtil::StartWith(path, mEnginePath);
	}
}