#pragma once
#include "UI/PanelWindow.h"
#include "UI/Child.h"
#include "UI/TreeNode.h"
#include "UI/Columns.h"
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
		void BuildVirtualFs(UI::TreeNode* root, std::filesystem::directory_entry entry, bool isEngineItem = false);

		/*
		* �����ʲ�����
		*/
		void BuildAssetGrid();
	private:
		std::string		mEnginePath;
		std::string		mAssetPath;
		std::string		mCurrentPath;
		UI::Child*		mVirtualFs{ nullptr };
		UI::Child*		mAssetGrid{ nullptr };
		UI::Columns*	mAssetGridColumns{ nullptr };
		const float		mGridPadding = 16.0f;
		const float		mThumbnailSize = 96.0f;
	};
}