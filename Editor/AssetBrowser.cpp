#include "AssetBrowser.h"
#include "UI/Button.h"
#include "UI/SameLine.h"
#include "UI/Separator.h"
#include "UI/TextSelectable.h"
#include "UI/DDSource.h"
#include "UI/DDTarget.h"
#include "UI/Group.h"

#include "FileItem.h"
#include "FolderItemContextualMenu.h"
#include "FileItemContextualMenu.h"

#include "Tools/StrUtil.h"
#include "Tools/MetaFile.h"

#include "Core/Texture.h"
#include "Core/TextureLoader.h"
#include "Core/TextureManger.h"
#include "Core/ServiceLocator.h"

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

		// 递归读取全部资产至资产管理器
		LoadAssets(std::filesystem::directory_entry(mEnginePath));
		LoadAssets(std::filesystem::directory_entry(mAssetPath));

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
	* 资产浏览器析构函数
	*/
	AssetBrowser::~AssetBrowser() {

	}

	void AssetBrowser::LoadAssets(std::filesystem::directory_entry entry) {
		bool isFolder = entry.is_directory();
		if (isFolder) {
			for (auto& item : std::filesystem::directory_iterator(entry)) {
				LoadAssets(item);
			}
			return;
		}

		std::string filepath = entry.path().string();
		std::string filename = entry.path().filename().string();
		std::string metaname = filename + ".meta";
		std::string metapath = filepath + ".meta";

		if (!std::filesystem::exists(metapath)) {
			return;
		}

		switch (Tool::StrUtil::GetFileType(filename))
		{
		case Tool::FileType::TEXTURE	:
		{
			// 读取meta文件
			Tool::MetaFile metaFile(metapath);
			int64_t uid = metaFile.Get<int64_t>("UID");

			// 创建资产
			Core::Texture* texture = new Core::Texture();
			texture->SetName(filename);
			texture->SetUID(uid);

			// 填充资产
			Core::TextureLoader::Create(filepath, *texture);

			// 注册资产
			CORESERVICE(Core::TextureManger).RegisterResource(texture);

			break;
		}
		case Tool::FileType::MODEL		:
			break;
		case Tool::FileType::AUDIO		:
			break;
		case Tool::FileType::SHADER		:
			break;
		case Tool::FileType::MATERIAL	:
			break;
		default:
			break;
		}
	}

	void AssetBrowser::UnloadAssets(std::filesystem::directory_entry entry) {
		bool isFolder = entry.is_directory();
		if (isFolder) {
			for (auto& item : std::filesystem::directory_iterator(entry)) {
				UnloadAssets(item);
			}
			return;
		}

		std::string filepath = entry.path().string();
		std::string filename = entry.path().filename().string();
		std::string metaname = filename + ".meta";
		std::string metapath = filepath + ".meta";

		if (!std::filesystem::exists(metapath)) {
			return;
		}

		switch (Tool::StrUtil::GetFileType(filename))
		{
		case Tool::FileType::TEXTURE:
		{
			// 通过名称注销资产
			CORESERVICE(Core::TextureManger).UnRegisterResource(filename);
			break;
		}
		case Tool::FileType::MODEL:
			break;
		case Tool::FileType::AUDIO:
			break;
		case Tool::FileType::SHADER:
			break;
		case Tool::FileType::MATERIAL:
			break;
		default:
			break;
		}
	}

	void AssetBrowser::BuildVirtualFs(FolderItem* root, std::filesystem::directory_entry entry, bool isEngineItem) {
		// 获取项目路径、名称、是否为文件夹
		std::string path = entry.path().string();
		std::string name = Tool::StrUtil::RemoveBasePath(path);
		bool isFolder = entry.is_directory();

		if (isFolder) {
			auto& treeNode = (root == nullptr) ? mVirtualFs->CreateWidget<FolderItem>(name, path) : root->CreateBrowserItem<FolderItem>(name, path);
			if (root == nullptr) {
				mEngineFolderItem = isEngineItem ? &treeNode : nullptr;
				mAssetFolderItem  = isEngineItem ? nullptr : &treeNode;
			}

			// 添加拖拽功能
			treeNode.CreatePlugin<UI::DDSource<BrowserItem*>>("Folder", name, &treeNode);

			// 根目录不可作为拖拽的源
			if (root == nullptr) {
				treeNode.DeleteAllPlugins();
			}

			// 添加PopupMenu
			auto& contextualMenu = treeNode.CreatePlugin<FolderItemContextualMenu>(name, path, isEngineItem);
			contextualMenu.BuildPopupContextItem();

			contextualMenu.itemAddedEvent	+= [this, &treeNode](const std::string& path) {
				BuildVirtualFs(&treeNode, std::filesystem::directory_entry(path), false);
			};

			contextualMenu.itemDeledEvent	+= [this, &treeNode]() {
				// 遍历文件夹，将资产从资产管理器中注销
				UnloadAssets(std::filesystem::directory_entry(treeNode.path));

				// 递归删除磁盘中的文件夹
				std::filesystem::remove_all(treeNode.path);

				// 为treeNode设置销毁标记，在下一帧绘制开始时销毁它
				treeNode.Destory();
			};

			contextualMenu.itemRenamedEvent += [this, &treeNode, root](const std::string& newName) {
				// 更新Key
				root->UpdateKey(treeNode.name, newName);

				// 更新treeNode
				treeNode.name = newName;
				treeNode.path = Tool::StrUtil::GetBasePath(treeNode.path) + '\\' + newName;

				for (auto& pair : treeNode.GetBrowserItems()) {
					pair.second->PropagatePath(treeNode.path);
				}
			};

			// 引擎文件和文件夹无法成为拖动的目标
			if (!isEngineItem) {
				treeNode.CreatePlugin<UI::DDTarget<BrowserItem*>>("Folder").dataReceivedEvent += [this, &treeNode](BrowserItem* data) {
					std::string prevPath = data->path;
					std::string name = Tool::StrUtil::RemoveBasePath(prevPath);
					std::string newPath = treeNode.path + '\\' + name;

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

						if (!engineItem) {
							data->Destory();
						}
					}
				};

				treeNode.CreatePlugin<UI::DDTarget<BrowserItem*>>("File").dataReceivedEvent += [this, &treeNode](BrowserItem* data) {
					std::string prevFilepath = data->path;
					std::string filename = Tool::StrUtil::RemoveBasePath(prevFilepath);
					std::string prevMetapath = prevFilepath + ".meta";
					std::string metaname = Tool::StrUtil::RemoveBasePath(prevMetapath);

					std::string newFilepath = treeNode.path + '\\' + filename;
					std::string newMetapath = treeNode.path + '\\' + metaname;

					bool engineItem = IsEngineItem(prevFilepath);

					if (prevFilepath == newFilepath) {
						return;
					}
					if (std::filesystem::exists(newFilepath) || !std::filesystem::exists(prevFilepath)) {
						return;
					}

					std::filesystem::copy_file(prevFilepath, newFilepath);
					std::filesystem::copy_file(prevMetapath, newMetapath);
					if (!engineItem) {
						std::filesystem::remove(prevFilepath);
						std::filesystem::remove(prevMetapath);
					}
					BuildVirtualFs(&treeNode, std::filesystem::directory_entry(newFilepath), false);

					if (!engineItem) {
						data->Destory();
					}
				};
			}

			for (auto& item : std::filesystem::directory_iterator(entry)) {
				BuildVirtualFs(&treeNode, item, isEngineItem);
			}
		}
		else {
			assert(root != nullptr);
			
			Tool::FileType fileType = Tool::StrUtil::GetFileType(path);
			if (fileType == Tool::FileType::UNSUPPORT) {
				return;
			}

			auto& leafNode = root->CreateBrowserItem<FileItem>(name, path);

			// 引擎文件和普通文件均可拖动
			leafNode.CreatePlugin<UI::DDSource<BrowserItem*>>("File", name, &leafNode);
		}
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