#pragma once
#include "UI/IWidget.h"
#include "Tools/Event.h"

namespace App {
	/*
	* 浏览器项目(是文件夹项目与文件项目的基类)
	*/
	class BrowserItem : public UI::IWidget {
	public:
		/*
		* 构造函数
		*/
		BrowserItem(const std::string& name, const std::string& path);
	public:
		std::string name;	// 项目名称
		std::string path;	// 项目路径
	};
}