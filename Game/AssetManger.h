#pragma once
#include "Renderer/Model.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderEngine.h"

namespace Game {

	/*
	* 管理游戏中使用到的预设资产，标准的几何模型与着色器等
	*/
	class AssetManger {
	public:
		AssetManger(Renderer::RenderEngine* renderEngine, const std::string& path);
		~AssetManger();

		Renderer::Model* GetModel(const std::string& name);

		Renderer::Mesh* GetMesh(const std::string& name);

	private:
		std::string mModelsPath;
		std::unordered_map<std::string, std::unique_ptr<Renderer::Model>> mModels;
	};

}