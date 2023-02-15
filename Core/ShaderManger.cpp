#include "ShaderManger.h"

namespace Core {
	/*
	* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
	*/
	Shader::Shader(const std::string & name)
	: IAsset(name) {}


	void Shader::Serialize(Tool::OutputMemoryStream& blob) const	{

	}

	void Shader::Deserialize(const Tool::InputMemoryStream& blob)	{

	}
}