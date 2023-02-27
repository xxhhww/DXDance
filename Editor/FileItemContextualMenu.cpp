#include "FileItemContextualMenu.h"

#include "UI/MenuList.h"
#include "UI/InputText.h"

#include "Tools/StrUtil.h"

#include "Core/Texture.h"
#include "Core/ServiceLocator.h"
#include "Core/TextureManger.h"

#include <filesystem>

namespace App {
	FileItemContextualMenu::FileItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem)
	: BrowserItemContextualMenu(name, path, isEngineItem) {}

	void FileItemContextualMenu::BuildPopupContextItem() {
		if (isProtected) {
			return;
		}

		// 一级目录
		auto& deleteMenu = CreateWidget<UI::MenuItem>("Delete");
		auto& renameMenu = CreateWidget<UI::MenuList>("Rename");

		// 二级目录
		auto& renameInputText = renameMenu.CreateWidget<UI::InputText>("##hidelabel", "");

		// 回调设置
		deleteMenu.clickedEvent += [this]() {
			itemDeledEvent.Invoke();
			Close();
		};

		renameInputText.enterPressedEvent += [this](const std::string& newName) {
			if (newName == name) {
				return;
			}

			std::string extension = Tool::StrUtil::GetFileExtension(name);
			// 避免资源重名
			int fails = 0;
			std::string correctName;
			switch (Tool::StrUtil::GetFileType(extension))
			{
			case Tool::FileType::TEXTURE:
			{
				do {
					correctName = !fails ? (newName + '.' + extension) : (newName + " (" + std::to_string(fails) + ")." + extension);
					fails++;
				} while (CORESERVICE(Core::TextureManger).IsRegistered(correctName));

				CORESERVICE(Core::TextureManger).RenameResource(name, correctName);
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

			std::string correctPath = Tool::StrUtil::GetBasePath(path) + '\\' + correctName;
			std::filesystem::rename(path, correctPath);
			std::filesystem::rename(path + ".meta", correctPath + ".meta");
			path = correctPath;
			name = newName + '.' + extension;

			itemRenamedEvent.Invoke(newName + '.' + extension);

			Close();
		};
	}
}