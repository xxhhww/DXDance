#include "FolderItemContextualMenu.h"

#include "FolderItem.h"

#include "UI/MenuList.h"
#include "UI/InputText.h"

#include "Tools/SystemCall.h"
#include "Tools/UIDGenerator.h"
#include "Tools/StrUtil.h"
#include "Tools/MetaFile.h"

#include "Windows/OpenFileDialog.h"

#include "Core/ServiceLocator.h"

#include <filesystem>

namespace App {
	FolderItemContextualMenu::FolderItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem)
	: BrowserItemContextualMenu(name, path, isEngineItem) {}

	void FolderItemContextualMenu::BuildPopupContextItem() {

		// 一级目录
		auto& showInFileBrowser = CreateWidget<UI::MenuItem>("Show in file browser");
		showInFileBrowser.clickedEvent += [this]() {
			Tool::SystemCalls::ShowInExplorer(path);
		};

		if (isProtected) {
			return;
		}

		// 一级目录
		auto& createMenu			= CreateWidget<UI::MenuList>("Create");
		auto& renameMenu			= CreateWidget<UI::MenuList>("Rename");
		auto& deleteMenu			= CreateWidget<UI::MenuItem>("Delete");
		auto& importMenu			= CreateWidget<UI::MenuItem>("Import");

		// 二级目录
		auto& createFolderMenu		= createMenu.CreateWidget<UI::MenuList>("Folder");
		auto& renameInputText		= renameMenu.CreateWidget<UI::InputText>("##hidelabel", "");

		// 三级目录
		auto& createFolderInputText = createFolderMenu.CreateWidget<UI::InputText>("##hidelabel", "");

		deleteMenu.clickedEvent += [this]() {
			itemDeledEvent.Invoke();
			Close();
		};

		importMenu.clickedEvent += [this]() {
			std::string modelFormats = "*.fbx;*.obj;*.pmx";
			std::string textureFormats = "*.png;*.jpeg;*.jpg;*.tga";
			std::string soundFormats = "*.mp3;*.ogg;*.wav;";

			Windows::OpenFileDialog selectAssetDialog("Select an asset to import");
			selectAssetDialog.AddFileType("Model (.fbx, .obj, .pmx)", modelFormats);
			selectAssetDialog.AddFileType("Texture (.png, .jpeg, .jpg, .tga)", textureFormats);
			selectAssetDialog.AddFileType("Sound (.mp3, .ogg, .wav)", soundFormats);
			selectAssetDialog.Show();

			if (selectAssetDialog.HasSucceeded()) {
				std::string filepath = selectAssetDialog.GetSelectedFilePath();
				std::string filenameExt = selectAssetDialog.GetSelectedFileName();	// 包含扩展名的文件名称
				std::string filename = Tool::StrUtil::RemoveExtension(filenameExt);	// 去除扩展名的文件名称
				std::string extension = Tool::StrUtil::GetFileExtension(filenameExt);

				switch (Tool::StrUtil::GetFileType(filenameExt))
				{
				case Tool::FileType::TEXTURE:
				{
					/*
					// 避免资源重名
					int fails = 0;
					std::string correctName;
					do {
						correctName = !fails ? filenameExt : filename + " (" + std::to_string(fails) + ")." + extension;
						fails++;
					} while (CORESERVICE(Core::TextureManger).IsRegistered(correctName));

					// 将资源复制到目标路径下
					std::string newPath = path + '\\' + correctName;
					std::filesystem::copy_file(filepath, newPath);

					// 创建资产
					Core::Texture* texture = new Core::Texture();
					texture->SetName(correctName);
					texture->SetUID(Tool::UIDGenerator::Get());

					// 读取纹理数据
					Core::TextureLoader::Create(newPath, *texture);

					// 创建并保存Meta文件
					Tool::MetaFile metaFile(newPath + ".meta");
					metaFile.Add<int64_t>("UID", texture->GetUID());
					metaFile.Save();

					// 注册资产
					CORESERVICE(Core::TextureManger).RegisterResource(texture);

					// 更新控件
					itemAddedEvent.Invoke(newPath);
					*/
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
					assert(false);
					break;
				}
			}

			Close();
		};

		renameInputText.enterPressedEvent += [this](const std::string& newName) {
			if (newName == name) {
				return;
			}

			int fails = 0;
			std::string correctPath;
			std::string basePath = Tool::StrUtil::GetBasePath(path) + '\\';
			do {
				correctPath = basePath + (!fails ? newName : newName + " (" + std::to_string(fails) + ')');
				++fails;
			} while (std::filesystem::exists(correctPath));

			std::filesystem::rename(path, correctPath);
			path = correctPath;
			name = newName;

			itemRenamedEvent.Invoke(newName);

			Close();
		};

		createFolderInputText.enterPressedEvent += [this](const std::string& name) {
			int fails = 0;
			std::string correctPath;
			std::string basePath = path + '\\';
			do {
				correctPath = basePath + (!fails ? name : name + " (" + std::to_string(fails) + ')');
				++fails;
			} while (std::filesystem::exists(correctPath));

			std::filesystem::create_directory(correctPath);

			itemAddedEvent.Invoke(correctPath);
			Close();
		};
	}
}