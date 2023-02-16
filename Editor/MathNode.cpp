#include "MathNode.h"

namespace App {
	bool MathNode::OnInputSlotTypeChanged(const std::vector<Slot*>& oppositeSlots) {
		bool outputTypeChanged{ false };

		SlotType targetType = SlotType::Float;
		for (const auto slotPtr : oppositeSlots) {
			if (slotPtr->type > targetType) {
				targetType = slotPtr->type;
			}
		}

		for (auto& slot : mInputSlots) {
			slot.type = targetType;
		}
		for (auto& slot : mOutputSlots) {
			if (slot.type != targetType) {
				slot.type = targetType;
				outputTypeChanged = true;
			}
		}
		return outputTypeChanged;
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
		mInputSlots.emplace_back(this, (id | (0 << 16) | InputFlag), "Left", SlotType::Float, false);
		mInputSlots.emplace_back(this, (id | (1 << 16) | InputFlag), "Right", SlotType::Float, false);

		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float, true);
	}

	bool Add::Draw() {
		bool isEdited{ false };
		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		ImGui::BeginGroup();
		for (auto& slot : mInputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		for (auto& slot : mOutputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImNodes::EndNode();
		ImGui::PopItemWidth();

		return isEdited;
	}

	Cos::Cos(int id)
	: MathNode(id, NodeType::Cos, "Cos") {
		mInputSlots.emplace_back(this, (id | (0 << 16) | InputFlag), "Left", SlotType::Float, false);

		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float, true);
	}

	bool Cos::Draw() {
		bool isEdited{ false };
		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		ImGui::BeginGroup();
		for (auto& slot : mInputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		for (auto& slot : mOutputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImNodes::EndNode();
		ImGui::PopItemWidth();

		return isEdited;
	}
};