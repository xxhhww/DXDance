#pragma once
#include "IAssetManger.h"
#include "ShaderManger.h"

namespace Core {
	class Material : public IAsset {
	public:
		/*
		* 默认构造函数，用于资产从文件中读取的情景
		*/
		inline Material() = default;

		/*
		* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
		*/
		Material(const std::string& name);

		/*
		* 默认析构函数
		*/
		inline ~Material() = default;

		void Serialize(Tool::OutputMemoryStream& blob) const	override;
		void Deserialize(const Tool::InputMemoryStream& blob)	override;
	private:
		int64_t mShaderID{ -1 };
		std::unordered_map<std::string, ShaderVar> mMaterialDatas;
	public:
	};

	class MaterialManger : public IAssetManger<Material> {
	public:
	private:
	};
}