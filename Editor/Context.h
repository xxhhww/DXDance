#pragma once
#include <string>
#include <memory>
#include "Windows/Window.h"
#include "Windows/InputManger.h"

namespace Editor {
	class Context {
	public:
		Context(const std::string& projPath, const std::string& projName);
		~Context();
	public:
		const std::string projectPath;
		const std::string projectName;
		const std::string projectAssetPath;
		const std::string projectShaderPath;
		const std::string projectMaterialPath;

		std::unique_ptr<Windows::Window> window;
		std::unique_ptr<Windows::InputManger> inputManger;
	};
}