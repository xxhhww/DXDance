#include "VariableNode.h"

namespace App {
	Float::Float(int id)
	: VariableNode<float>(id, NodeType::Float, "Float") {
		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float, true);
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

	Float2::Float2(int id)
	: VariableNode<Math::Vector2>(id, NodeType::Float2, "Float2") {
		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float2, true);
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
		ImGui::DragFloat("##hidelabel1", &mValue.x, 0.01f, mMin.x, mMax.x);
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat("##hidelabel2", &mValue.y, 0.01f, mMin.y, mMax.y);
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
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

	Color::Color(int id) 
	: VariableNode<Math::Color>(id, NodeType::Color, "Color") {
		mOutputSlots.emplace_back(this, (id | (0 << 16) | OutputFlag), "Result", SlotType::Float3, true);
	}

	/*
	* 在NodeEditor中绘制图形
	*/
	bool Color::Draw() {
		bool isEdited{ false };
		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		// Title Bar
		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		ImGui::BeginGroup();
		ImGui::ColorEdit3("##hidelabel", reinterpret_cast<float*>(&mValue));
		isEdited = ImGui::IsItemDeactivatedAfterEdit();
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
}