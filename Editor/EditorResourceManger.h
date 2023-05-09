#pragma once
#include "Renderer/Model.h"
#include "Renderer/Shader.h"

namespace App {

	class EditorResourceManger {
	public:
		EditorResourceManger(const std::string& path);
		~EditorResourceManger();

		Renderer::Model* GetModel(const std::string& name);

	private:
		std::string mModelsPath;
		std::unordered_map<std::string, std::unique_ptr<Renderer::Model>> mModels;
	};

}