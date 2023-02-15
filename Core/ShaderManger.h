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
		* Ĭ�Ϲ��캯���������ʲ����ļ��ж�ȡ���龰
		*/
		inline Shader() = default;

		/*
		* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
		*/
		Shader(const std::string& name);

		/*
		* Ĭ����������
		*/
		inline ~Shader() = default;

		void Serialize(Tool::OutputMemoryStream& blob) const	override;
		void Deserialize(const Tool::InputMemoryStream& blob)	override;
	private:
		std::string mHLSLCode{ "" };							// HLSL����
		Tool::OutputMemoryStream mGraphBlob{ "" };				// ShaderGraph����
		std::unordered_map<std::string, ShaderVar> mDataLayout;	// ConstantBuffer����
	};

	class ShaderManger : public IAssetManger<Shader> {
	public:
	private:
	};
}