#include "MaterialManger.h"
#include "ServiceLocator.h"

namespace Core {
	/*
	* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
	*/
	Material::Material(const std::string& name) 
	: IAsset(name) {
	}

	void Material::Serialize(Tool::OutputMemoryStream& blob) const	{

	}
	void Material::Deserialize(const Tool::InputMemoryStream& blob) {

	}
}