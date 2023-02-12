#include "VariableNode.h"

namespace UI {
	Float::Float(int id)
	: VariableNode<float>(id, NodeType::Float, "Float") {
		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float);
	}

	bool Float::Draw() {
		bool isEdited{ false };
		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		// Title Bar
		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		ImGui::BeginGroup();
		ImGui::DragFloat("##hidelabel", &mValue, 0.01f, mMin, mMax);
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::EndGroup();

		ImGui::BeginGroup();
		for (auto& slot : mOutputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImNodes::EndNode();
		ImGui::PopItemWidth();

		return isEdited;
	}

	Float2::Float2(int id)
	: VariableNode<Math::Vector2>(id, NodeType::Float2, "Float2") {
		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float2);
	}

	bool Float2::Draw() {
		bool isEdited{ false };
		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		// Title Bar
		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		ImGui::BeginGroup();
		ImGui::DragFloat("##hidelabel", &mValue.x, 0.01f, mMin.x, mMax.x);
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat("##hidelabel", &mValue.y, 0.01f, mMin.y, mMax.y);
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::EndGroup();

		ImGui::BeginGroup();
		for (auto& slot : mOutputSlots) {
			slot.Draw();
		}
		ImGui::EndGroup();

		ImNodes::EndNode();
		ImGui::PopItemWidth();

		return isEdited;
	}
}