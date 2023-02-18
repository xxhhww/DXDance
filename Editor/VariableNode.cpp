#include "VariableNode.h"
#include "UI/Group.h"
#include "UI/SameLine.h"

namespace App {
	Float::Float(int id)
	: VariableNode(id, NodeType::Float, "Float") {
		auto& group = CreateWidget<UI::Group>();
		mWidget = &group.CreateWidget<UI::DragFloat>("##hidelabel");
		mWidget->editCompletedEvent += [this](const float& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "X");
	}

	Float2::Float2(int id)
	: VariableNode(id, NodeType::Float2, "Float2") {
		auto& group = CreateWidget<UI::Group>();
		mWidget = &group.CreateWidget<UI::DragFloat2Split>("##hidelabel");
		mWidget->editCompletedEvent += [this](const float& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "X");
		EmplaceOutputPin(&outputGroup, PinType::Float, "Y");
		EmplaceOutputPin(&outputGroup, PinType::Float2, "XY");
	}

	Float3::Float3(int id)
		: VariableNode(id, NodeType::Float3, "Float3") {
		auto& group = CreateWidget<UI::Group>();
		mWidget = &group.CreateWidget<UI::DragFloat3Split>("##hidelabel");
		mWidget->editCompletedEvent += [this](const float& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "X");
		EmplaceOutputPin(&outputGroup, PinType::Float, "Y");
		EmplaceOutputPin(&outputGroup, PinType::Float, "Z");
		EmplaceOutputPin(&outputGroup, PinType::Float3, "XYZ");
	}

	Float4::Float4(int id)
		: VariableNode(id, NodeType::Float4, "Float4") {
		auto& group = CreateWidget<UI::Group>();
		mWidget = &group.CreateWidget<UI::DragFloat4Split>("##hidelabel");
		mWidget->editCompletedEvent += [this](const float& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "X");
		EmplaceOutputPin(&outputGroup, PinType::Float, "Y");
		EmplaceOutputPin(&outputGroup, PinType::Float, "Z");
		EmplaceOutputPin(&outputGroup, PinType::Float, "W");
		EmplaceOutputPin(&outputGroup, PinType::Float4, "XYZW");
	}

	Color::Color(int id) 
	: VariableNode(id, NodeType::Color, "Color") {
		auto& group = CreateWidget<UI::Group>();
		mWidget = &group.CreateWidget<UI::ColorEdit>("##hidelabel");
		mWidget->editCompletedEvent += [this](const Math::Color& value) {
			this->mIsEdited = true;
		};

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float3, "Result");
	}

	void Float::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mWidget->data);
		blob.Write(mWidget->min);
		blob.Write(mWidget->max);
		blob.Write(name);
		blob.Write(isExposed);
	}

	void Float::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mWidget->data);
		blob.Read(mWidget->min);
		blob.Read(mWidget->max);
		blob.Read(name);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}

	void Float2::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mWidget->data);
		blob.Write(mWidget->min);
		blob.Write(mWidget->max);
		blob.Write(name);
		blob.Write(isExposed);
	}

	void Float2::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mWidget->data);
		blob.Read(mWidget->min);
		blob.Read(mWidget->max);
		blob.Read(name);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}

	void Float3::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mWidget->data);
		blob.Write(mWidget->min);
		blob.Write(mWidget->max);
		blob.Write(name);
		blob.Write(isExposed);
	}

	void Float3::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mWidget->data);
		blob.Read(mWidget->min);
		blob.Read(mWidget->max);
		blob.Read(name);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}

	void Float4::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mWidget->data);
		blob.Write(mWidget->min);
		blob.Write(mWidget->max);
		blob.Write(name);
		blob.Write(isExposed);
	}

	void Float4::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mWidget->data);
		blob.Read(mWidget->min);
		blob.Read(mWidget->max);
		blob.Read(name);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}

	void Color::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
		blob.Write(mWidget->data);
		blob.Write(name);
		blob.Write(isExposed);
	}

	void Color::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		blob.Read(mWidget->data);
		blob.Read(name);
		blob.Read(isExposed);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}
}