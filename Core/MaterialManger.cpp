#include "MaterialManger.h"
#include "ServiceLocator.h"

namespace Core {
	/*
	* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
	*/
	Material::Material(const std::string& name) 
	: IAsset(name) {
	}

	void Material::Serialize(Tool::OutputMemoryStream& blob) const	{

	}
	void Material::Deserialize(const Tool::InputMemoryStream& blob) {

	}
}