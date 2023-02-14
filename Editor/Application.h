#pragma once
#include "Editor.h"
#include "Context.h"
#include <string>

namespace App {
	class Application {
	public:
		/*
		* 初始化引擎上下文与编辑器
		*/
		Application(const std::string& projPath = "E:\\DXDanceProj", const std::string& projName = "DXDacneTest");

		/*
		* 默认析构函数
		*/
		~Application() = default;

		/*
		* 运行Application
		*/
		int Run();
	private:
		Context mContext;
		Editor mEditor;
	};
}