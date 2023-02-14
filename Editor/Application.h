#pragma once
#include "Editor.h"
#include "Context.h"
#include <string>

namespace Editor {
	class Application {
	public:
		// TODO：后续由ProjHub进行初始化
		Application(const std::string& projPath = "E:\\DXDanceProj", const std::string& projName = "DXDacneTest");
		~Application() = default;
	private:
		
		Editor mEditor;
	};
}