#pragma once
#include "IAssetManger.h"
#include "ShaderManger.h"

namespace Core {
	class Material : public IAsset {
	public:
		/*
		* Ĭ�Ϲ��캯���������ʲ����ļ��ж�ȡ���龰
		*/
		inline Material() = default;

		/*
		* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
		*/
		Material(const std::string& name);

		/*
		* Ĭ����������
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