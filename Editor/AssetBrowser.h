#pragma once
#include "UI/PanelWindow.h"
#include "UI/Group.h"
#include "UI/Child.h"
#include "UI/TreeNode.h"
#include <filesystem>

namespace App {
	/*
	* 资产浏览器
	*/
	class AssetBrowser : public UI::PanelWindow {
	public:
		/*
		* 资产浏览器构造函数
		*/
		AssetBrowser(
			const std::string& title = "",
			const std::string& enginePath = "",
			const std::string& assetPath = "",
			bool opened = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);

		/*
		* 资产浏览器析构函数
		*/
		~AssetBrowser();

	private:
		/*
		* 递归函数，处理项目路径下的文件夹或者目录
		*/
		void ConsiderItem(UI::TreeNode* root, std::filesystem::directory_entry entry, bool isEngineItem = false);
	private:
		std::string mEnginePath;
		std::string mAssetPath;

		UI::Child* mVirtualFolder{ nullptr };
		UI::Child* mAssetGrid{ nullptr };
	};
}