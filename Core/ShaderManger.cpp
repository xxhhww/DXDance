#include "ShaderManger.h"

namespace Core {
	/*
	* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
	*/
	Shader::Shader(const std::string & name)
	: IAsset(name) {}


	void Shader::Serialize(Tool::OutputMemoryStream& blob) const	{

	}

	void Shader::Deserialize(const Tool::InputMemoryStream& blob)	{

	}
}