#include "FolderItemContextualMenu.h"
#include "UI/MenuList.h"
#include "UI/InputText.h"
#include "Tools/SystemCall.h"
#include "Windows/OpenFileDialog.h"
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
		auto& createMenu = CreateWidget<UI::MenuList>("Create");
		auto& deleteMenu = CreateWidget<UI::MenuItem>("Delete");
		auto& importMenu = CreateWidget<UI::MenuItem>("Import");

		deleteMenu.clickedEvent += [this]() {
			itemDeledEvent.Invoke(path);
			Close();
		};

		importMenu.clickedEvent += [this]() {
			std::string modelFormats = "*.fbx;*.obj;";
			std::string textureFormats = "*.png;*.jpeg;*.jpg;*.tga";
			std::string shaderFormats = "*.glsl;";
			std::string soundFormats = "*.mp3;*.ogg;*.wav;";

			Windows::OpenFileDialog selectAssetDialog("Select an asset to import");
			selectAssetDialog.AddFileType("Any supported format", modelFormats + textureFormats + shaderFormats + soundFormats);
			selectAssetDialog.AddFileType("Model (.fbx, .obj)", modelFormats);
			selectAssetDialog.AddFileType("Texture (.png, .jpeg, .jpg, .tga)", textureFormats);
			selectAssetDialog.AddFileType("Shader (.glsl)", shaderFormats);
			selectAssetDialog.AddFileType("Sound (.mp3, .ogg, .wav)", soundFormats);
			selectAssetDialog.Show();

			if (selectAssetDialog.HasSucceeded()) {
				std::string source = selectAssetDialog.GetSelectedFilePath();
				std::string filename = selectAssetDialog.GetSelectedFileName();
				int i = 32;
			}
		};

		// 二级目录
		auto& createFolderMenu = createMenu.CreateWidget<UI::MenuList>("Folder");

		// 三级目录
		auto& createFolderInputText = createFolderMenu.CreateWidget<UI::InputText>("##hidelabel", "");

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