#pragma once
#include "Editor.h"
#include "Context.h"
#include <string>

namespace Editor {
	class Application {
	public:
		// TODO��������ProjHub���г�ʼ��
		Application(const std::string& projPath = "E:\\DXDanceProj", const std::string& projName = "DXDacneTest");
		~Application() = default;
	private:
		
		Editor mEditor;
	};
}