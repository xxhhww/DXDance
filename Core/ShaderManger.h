#pragma once
#include "IAssetManger.h"
#include <variant>

namespace Core {
	using Var = std::variant<bool, int, float>;

	enum class ShaderVar {
		Bool, Int, Float, Vec2, Vec3, Vec4, Color
	};

	enum class ShaderVarUI {
		Drag, Slider, Input, Color
	};

	struct ShaderDataDesc {
		ShaderVar	var;
		ShaderVarUI ui;
	};

	class Shader : public IAsset {
	public:
		Shader() = default;
		Shader(const std::string& name);
		~Shader() = default;

		void SetHLSLCode(const std::string& code);
		void SetGraphBlob(const std::string& code);
		const auto& GetHLSLCode() const;
		const auto& GetGraphBlob() const;
	public:	// [Json Serialize]
		void Serialize(rapidjson::Document& outputDoc) override;
		void Deserialize(const rapidjson::Document& inputDoc) override;
	private:
		std::string mHLSLCode;	// HLSL代码
		std::string mGraphBlob;	// ShaderGraph数据
		std::unordered_map<std::string, ShaderDataDesc> mDataLayout;	// ConstantBuffer布局
	};

	class ShaderManger : public IAssetManger<Shader> {
	public:
	private:
	};
}