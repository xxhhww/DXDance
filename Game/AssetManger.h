#pragma once
#include "Renderer/Model.h"
#include "Renderer/Mesh.h"

namespace Game {

	/*
	* 管理游戏中使用到的资产
	*/
	class AssetManger {
	public:
		AssetManger(const std::string& path);
		~AssetManger();

		Renderer::Model* GetModel(const std::string& name);

		Renderer::Mesh* GetMesh(const std::string& name);

	private:
		std::string mModelsPath;
		std::unordered_map<std::string, std::unique_ptr<Renderer::Model>> mModels;
	};

}