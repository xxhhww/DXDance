#pragma once
#include "UI/IWidgetContainer.h"
#include "UI/Group.h"
#include "UI/Text.h"
#include "UI/InputText.h"
#include "UI/DragFloat.h"
#include "UI/DragFloat2.h"
#include "UI/DragFloat3.h"
#include "UI/DragFloat4.h"
#include "UI/SliderFloat.h"
#include "UI/SliderFloat2.h"
#include "UI/SliderFloat3.h"
#include "UI/SliderFloat4.h"

#include "Tools/ISerializable.h"

namespace ECS {

	class IComponent : public Tool::ISerializable {
	public:
		virtual ~IComponent() = default;
		
		/*
		* ��Inspector���ã��������������ص�д��UI������
		*/
		virtual void OnInspector(UI::IWidgetContainer* container) {};
	};

}