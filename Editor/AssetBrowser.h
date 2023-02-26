#pragma once
#include "UI/PanelWindow.h"
#include "UI/Child.h"
#include "UI/Columns.h"
#include "UI/Text.h"
#include "FolderItem.h"
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
		* �ݹ麯������ȡ��ע����Ŀ·���µ�ȫ���ʲ�
		*/
		void LoadAssets(std::filesystem::directory_entry entry);

		/*
		* �ݹ麯������·���µ�ȫ���ʲ�ע��
		*/
		void UnloadAssets(std::filesystem::directory_entry entry);

		/*
		* �ݹ麯����������Ŀ·���µ��ļ��л���Ŀ¼
		*/
		void BuildVirtualFs(FolderItem* root, std::filesystem::directory_entry entry, bool isEngineItem = false);

		/*
		* ���������ļ�����ͼ
		*/
		void BuildVirtualFsView(FolderItem* node);

		/*
		* �ж��ļ�·���Ƿ�Ϊ�����ļ�����Ϊƥ���ַ���ǰ׺
		*/
		bool IsEngineItem(const std::string& viewPath);
	private:
		std::string		mEnginePath;
		std::string		mAssetPath;
		
		UI::Child*		mVirtualFs{ nullptr };			// �����ļ����Ӵ���
		FolderItem*		mEngineFolderItem{ nullptr };	// �����ļ��ĸ���Ŀ
		FolderItem*		mAssetFolderItem{ nullptr };	// �ʲ��ļ��ĸ���Ŀ

		UI::Child*		mFolderItemView{ nullptr };		// �����ļ�ϵͳ��ͼ
		UI::Text*		mViewPathText{ nullptr };		// ��ʾ��ͼ��ǰչʾ��·��
		UI::Columns*	mViewColumns{ nullptr };		// ��ͼ��չʾ��
		FolderItem*		mCurrFolderItem{ nullptr };		// ��ǰչʾ���ļ�����Ŀ
		const float		mGridPadding = 16.0f;
		const float		mThumbnailSize = 96.0f;
	};
}