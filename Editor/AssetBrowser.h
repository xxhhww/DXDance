#pragma once
#include "UI/PanelWindow.h"
#include "UI/Group.h"
#include "UI/Columns.h"

namespace App {
	/*
	* �ʲ������
	*/
	class AssetBrowser : public UI::PanelWindow {
	public:
		/*
		* �ʲ���������캯��
		*/
		AssetBrowser(
			const std::string& title = "",
			const std::string& enginePath = "",
			const std::string& assetPath = "",
			bool opened = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);

		/*
		* �ʲ��������������
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