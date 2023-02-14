#pragma once
#include "IAssetManger.h"
#include "ShaderManger.h"

namespace Core {
	class Material : public IAsset {
	public:
		Material() = default;
		Material(const std::string& name) : IAsset(name) {}
		~Material() = default;

		void SetShader(int64_t shaderID);
	private:
		int64_t mShaderID{ -1 };
		std::unordered_map<std::string, Var> mMaterialDatas;
	public:
	};

	class MaterialManger : public IAssetManger<Material> {
	public:
	private:
	};
}