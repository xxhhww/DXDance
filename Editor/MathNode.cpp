#include "MathNode.h"
#include "UI/imnodes.h"
#include "UI/Group.h"
#include "UI/SameLine.h"

namespace App {
	bool MathNode::OnInputPinTypeChanged(const std::vector<Pin*>& oppositePins) {
		bool typeChanged{ false };

		PinType targetType = PinType::Float;
		for (const auto pin : oppositePins) {
			if (pin->type > targetType) {
				targetType = pin->type;
			}
		}

		for (auto& pin : mInputPins) {
			pin->type = targetType;
		}
		for (auto& pin : mOutputPins) {
			if (pin->type != targetType) {
				pin->type = targetType;
				typeChanged = true;
			}
		}
		return typeChanged;
	}

	void MathNode::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(mNodeType);
		blob.Write(objectID);
		blob.Write(mPosition);
	}

	void MathNode::Deserialize(Tool::InputMemoryStream& blob) {
		blob.Read(mPosition);
		ImNodes::SetNodeEditorSpacePos(objectID, ImVec2(mPosition.x, mPosition.y));
	}

	Add::Add(int id)
	: MathNode(id, NodeType::Add, "Add") {
		auto& inputGroup = CreateWidget<UI::Group>();
		EmplaceInputPin(&inputGroup, PinType::Float, "Left");
		EmplaceInputPin(&inputGroup, PinType::Float, "Right");

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "Result");
	}

	Cos::Cos(int id)
	: MathNode(id, NodeType::Cos, "Cos") {
		auto& inputGroup = CreateWidget<UI::Group>();
		EmplaceInputPin(&inputGroup, PinType::Float, "Left");

		CreateWidget<UI::SameLine>();

		auto& outputGroup = CreateWidget<UI::Group>();
		EmplaceOutputPin(&outputGroup, PinType::Float, "Result");
	}
};