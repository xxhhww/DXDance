#pragma once
#include "UI/PanelWindow.h"
#include "UI/Child.h"
#include "UI/Columns.h"
#include "UI/Text.h"
#include "FolderItem.h"
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
		* 递归函数，读取并注册项目路径下的全部资产
		*/
		void LoadAssets(std::filesystem::directory_entry entry);

		/*
		* 递归函数，将路径下的全部资产注销
		*/
		void UnloadAssets(std::filesystem::directory_entry entry);

		/*
		* 递归函数，处理项目路径下的文件夹或者目录
		*/
		void BuildVirtualFs(FolderItem* root, std::filesystem::directory_entry entry, bool isEngineItem = false);

		/*
		* 绘制虚拟文件夹视图
		*/
		void BuildVirtualFsView(FolderItem* node);

		/*
		* 判断文件路径是否为引擎文件，即为匹配字符串前缀
		*/
		bool IsEngineItem(const std::string& viewPath);
	private:
		std::string		mEnginePath;
		std::string		mAssetPath;
		
		UI::Child*		mVirtualFs{ nullptr };			// 虚拟文件夹子窗口
		FolderItem*		mEngineFolderItem{ nullptr };	// 引擎文件的根项目
		FolderItem*		mAssetFolderItem{ nullptr };	// 资产文件的根项目

		UI::Child*		mFolderItemView{ nullptr };		// 虚拟文件系统视图
		UI::Text*		mViewPathText{ nullptr };		// 显示视图当前展示的路径
		UI::Columns*	mViewColumns{ nullptr };		// 视图的展示列
		FolderItem*		mCurrFolderItem{ nullptr };		// 当前展示的文件夹项目
		const float		mGridPadding = 16.0f;
		const float		mThumbnailSize = 96.0f;
	};
}