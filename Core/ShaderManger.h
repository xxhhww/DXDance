#pragma once
#include <variant>
#include "IAssetManger.h"
#include "Math/Vector.h"
#include "Math/Color.h"

namespace Core {
	using TextureID = int64_t;
	using ShaderVar = std::variant<bool, float, Math::Vector2, Math::Vector3, Math::Vector4, Math::Color, TextureID>;

	class Shader : public IAsset {
	public:
		/*
		* 默认构造函数，用于资产从文件中读取的情景
		*/
		inline Shader() = default;

		/*
		* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
		*/
		Shader(const std::string& name);

		/*
		* 默认析构函数
		*/
		inline ~Shader() = default;

		void Serialize(Tool::OutputMemoryStream& blob) const	override;
		void Deserialize(const Tool::InputMemoryStream& blob)	override;
	private:
		std::string mHLSLCode{ "" };							// HLSL代码
		Tool::OutputMemoryStream mGraphBlob{ "" };				// ShaderGraph数据
		std::unordered_map<std::string, ShaderVar> mDataLayout;	// ConstantBuffer布局
	};

	class ShaderManger : public IAssetManger<Shader> {
	public:
	private:
	};
}