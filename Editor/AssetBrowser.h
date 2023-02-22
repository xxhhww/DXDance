#pragma once
#include "UI/PanelWindow.h"
#include "UI/Group.h"
#include "UI/Columns.h"

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
		std::string mEnginePath;
		std::string mAssetPath;

		UI::Columns<2>* mColumns{ nullptr };
		UI::Group* mVirtualFolder{ nullptr };
		UI::Group* mAssetList{ nullptr };
	};
}