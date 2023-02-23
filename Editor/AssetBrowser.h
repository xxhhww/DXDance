#pragma once
#include "UI/PanelWindow.h"
#include "UI/Group.h"
#include "UI/Child.h"
#include "UI/TreeNode.h"
#include <filesystem>

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
		/*
		* �ݹ麯����������Ŀ·���µ��ļ��л���Ŀ¼
		*/
		void ConsiderItem(UI::TreeNode* root, std::filesystem::directory_entry entry, bool isEngineItem = false);
	private:
		std::string mEnginePath;
		std::string mAssetPath;

		UI::Child* mVirtualFolder{ nullptr };
		UI::Child* mAssetGrid{ nullptr };
	};
}