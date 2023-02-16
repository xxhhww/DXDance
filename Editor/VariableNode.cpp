#include "VariableNode.h"
#include "UI/Group.h"
#include "UI/SameLine.h"
#include "UI/DragFloat.h"
#include "UI/DragFloat2.h"
#include "UI/DragFloat3.h"
#include "UI/DragFloat4.h"
#include "UI/ColorEdit.h"

namespace App {
	Float::Float(int id)
	: VariableNode<float>(id, NodeType::Float, "Float") {
		auto& group = CreateWidget<UI::Group>();
		auto& drag = group.CreateWidget<UI::DragFloat>("##hidelabel", mValue, mMin, mMax);
		drag.editCompletedEvent += [this](const float& value) {
			this->mIsEdited = true;
		};
	
		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "Result");
	}

	Float2::Float2(int id)
	: VariableNode<Math::Vector2>(id, NodeType::Float2, "Float2") {
	}

	Color::Color(int id) 
	: VariableNode<Math::Color>(id, NodeType::Color, "Color") {
		auto& group = CreateWidget<UI::Group>();
		auto& colorEdit = group.CreateWidget<UI::ColorEdit>("##hidelabel", mValue);
		colorEdit.editCompletedEvent += [this](const Math::Color& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float3, "Result");
	}
}