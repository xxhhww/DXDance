#include "Node.h"
#include "VariableNode.h"
#include "MathNode.h"
#include "UI/imnodes.h"

namespace App {
	Node::Node(int id, NodeType nodeType, const std::string& label)
	: objectID(id)
	, mNodeType(nodeType)
	, mLabel(label) {}

	bool Node::Draw() {
		mIsEdited = false;

		ImGui::PushItemWidth(width);
		ImNodes::BeginNode(objectID);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(mLabel.c_str());
		ImNodes::EndNodeTitleBar();

		// 绘制控件，可能更新mIsEdited变量
		DrawWidgets();

		ImNodes::EndNode();
		ImGui::PopItemWidth();

		return mIsEdited;
	}

	bool Node::OnInputPinTypeChanged(const std::vector<Pin*>& oppositePins) {
		return false;
	}

	void Node::SetInputPinLinked(int index, int inLinkID) {
		mInputPins[index]->isLinked = true;
		mInputPins[index]->inLinkID = inLinkID;
	}

	void Node::SetInputPinBroken(int index) {
		mInputPins[index]->isLinked = false;
		mInputPins[index]->inLinkID = -1;
	}

	void Node::SetOutputPinLinked(int index) {
		mOutputPins[index]->isLinked = true;
	}

	void Node::SetOutputPinBroken(int index) {
		mOutputPins[index]->isLinked = false;
	}

	void Node::SetPosition(float x, float y) {
		mPosition = Math::Vector2(x, y);
	}

	std::unique_ptr<Node> Node::CreateNode(int id, NodeType nodeType) {
		switch (nodeType) {
		case App::NodeType::Float:			return std::make_unique<Float>(id);
		case App::NodeType::Float2:			return std::make_unique<Float2>(id);
		case App::NodeType::Float3:			
		case App::NodeType::Float4:			
		case App::NodeType::Bool:
		case App::NodeType::Color:			return std::make_unique<Color>(id);
		case App::NodeType::SamplerState:	
		case App::NodeType::Texture2D:		return nullptr;
		case App::NodeType::Add:			return std::make_unique<Add>(id);
		case App::NodeType::Cos:			return std::make_unique<Cos>(id);
		case App::NodeType::BRDF:			return nullptr;
		}
		assert(false);
		return nullptr;
	}

	/*
	* 创建输入Pin
	*/
	void Node::EmplaceInputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label) {
		auto& pin = container->CreateWidget<Pin>(this, (objectID | (mInputPins.size() << 16) | InputFlag), false, slotType, label);
		mInputPins.push_back(&pin);
	}

	/*
	* 创建输出Pin
	*/
	void Node::EmplaceOutputPin(UI::IWidgetContainer* container, PinType slotType, const std::string& label) {
		auto& pin = container->CreateWidget<Pin>(this, (objectID | (mOutputPins.size() << 16) | OutputFlag), true, slotType, label);
		mOutputPins.push_back(&pin);
	}
}