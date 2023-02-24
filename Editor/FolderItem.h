#pragma once
#include "BrowserItem.h"
#include <map>

namespace App {
	/*
	* 文件夹项目
	*/
	class FolderItem : public BrowserItem {
	public:
		/*
		* 构造函数
		*/
		FolderItem(const std::string& name, const std::string& path);

		/*
		* 创建浏览器项目
		*/
		template<typename T, typename ...Args>
		inline T& CreateBrowserItem(Args&&... args) {
			static_assert(std::is_base_of<BrowserItem, T>::value, "T should derive from BrowserItem");
			T* item = new T(args...);
			mBrowserItems.emplace(item->name, item);
			return *item;
		}

		/*
		* 删除被标记为销毁的Widget
		*/
		void DoDestruction();
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> openedEvent;
		Tool::Event<> closedEvent;
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;

		bool opened{ false };	// 文件夹项目是否打开
	protected:
		std::map<std::string, std::unique_ptr<BrowserItem>> mBrowserItems;	// 子项目
	};
}