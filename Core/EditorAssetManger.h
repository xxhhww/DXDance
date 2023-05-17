#pragma once
#include "Renderer/Model.h"
#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"

namespace Core {

	class EditorAssetManger {
	public:
		EditorAssetManger(const std::string& path);
		~EditorAssetManger();

		Renderer::Model* GetModel(const std::string& name);

		Renderer::Mesh* GetMesh(const std::string& name);

		Renderer::GraphicsShader* GetShader(const std::string& name);

	private:
		std::string mModelsPath;
		std::unordered_map<std::string, std::unique_ptr<Renderer::Model>> mModels;
		std::unordered_map<std::string, Renderer::GraphicsShader*> mGraphicsShaders;
	};

}