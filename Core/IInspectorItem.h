#pragma once
#include "Tools/ISerializable.h"

namespace Core {

	/*
	* ����ʾ�����Դ�����
	*/
	class IInspectorItem : public Tool::ISerializable {
	public:
		virtual ~IInspectorItem() = default;
	};

}