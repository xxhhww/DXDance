#pragma once
#include "BrowserItem.h"
#include <map>

namespace App {
	/*
	* �ļ�����Ŀ
	*/
	class FolderItem : public BrowserItem {
	public:
		/*
		* ���캯��
		*/
		FolderItem(const std::string& name, const std::string& path);

		/*
		* �����������Ŀ
		*/
		template<typename T, typename ...Args>
		inline T& CreateBrowserItem(Args&&... args) {
			static_assert(std::is_base_of<BrowserItem, T>::value, "T should derive from BrowserItem");
			T* item = new T(args...);
			mBrowserItems.emplace(item->name, item);
			return *item;
		}

		/*
		* ɾ�������Ϊ���ٵ�Widget
		*/
		void DoDestruction();
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> openedEvent;
		Tool::Event<> closedEvent;
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;

		bool opened{ false };	// �ļ�����Ŀ�Ƿ��
	protected:
		std::map<std::string, std::unique_ptr<BrowserItem>> mBrowserItems;	// ����Ŀ
	};
}