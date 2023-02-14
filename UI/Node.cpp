#include "Node.h"

namespace UI {
	// Helper Function

	// 获取类型提示
	static std::unordered_map<SlotType, std::string> sTypeHintingMap = {
		{ SlotType::Bool		, {"(b)"}		},
		{ SlotType::Float		, {"(1)"}		},
		{ SlotType::Float2		, {"(2)"}		},
		{ SlotType::Float3		, {"(3)"}		},
		{ SlotType::Float4		, {"(4)"}		},
		{ SlotType::SamplerState, {"(ss)"}		},
		{ SlotType::Texture2D	, {"(tex2d)"}	},
	};
	std::string GetTypeHinting(SlotType type) {
		return sTypeHintingMap.at(type);
	}
}

namespace UI {
	ImnodeObject::ImnodeObject(int id)
	: objectID(id) {}

	Slot::Slot(Node* node, int id, const std::string& label, SlotType slotType) 
	: ImnodeObject(id)
	, ownNode(node) 
	, type(slotType) 
	, mLabel(label) {}

	bool Slot::Draw() {
		std::string hintingStr = mLabel + GetTypeHinting(type);

		if (!isOutput) {
			ImNodes::BeginInputAttribute(objectID);
			ImGui::TextUnformatted(hintingStr.c_str());
			ImNodes::EndInputAttribute();

		}
		else {
			ImNodes::BeginOutputAttribute(objectID);
			const float label_width = ImGui::CalcTextSize(hintingStr.c_str()).x;
			ImGui::Indent(ownNode->width - label_width);
			ImGui::TextUnformatted(hintingStr.c_str());
			ImNodes::EndOutputAttribute();
		}
		return false;
	}

	bool Link::Draw() {
		ImNodes::Link(objectID, fromSlot, toSlot);
		return false;
	}

	int Link::FromNodeID() const { 
		return fromSlot & 0x0000FFFF; 
	}

	int Link::ToNodeID() const { 
		return toSlot & 0x0000FFFF; 
	}

	int Link::FromSlotIndex() const { 
		return (fromSlot >> 16) & 0x000000FF; 
	}

	int Link::ToSlotIndex() const { 
		return (toSlot >> 16) & 0x000000FF; 
	}

	void Link::Serialize(Tool::OutputMemoryStream& blob) {
		blob.Write(objectID);
		blob.Write(fromSlot);
		blob.Write(toSlot);
	}

	void Link::Deserialize(Tool::InputMemoryStream& blob) {
		// objectID已在外部读取完毕
		blob.Read(fromSlot);
		blob.Read(toSlot);
	}

	bool Node::OnInputSlotTypeChanged(const std::vector<Slot*>& oppositeSlots) {
		return false;
	}

	void Node::SetInputSlotLinked(int index, int inLinkID) {
		mInputSlots[index].isLinked = true;
		mInputSlots[index].inLinkID = inLinkID;
	}

	void Node::SetInputSlotBroken(int index) {
		mInputSlots[index].isLinked = false;
		mInputSlots[index].inLinkID = -1;
	}

	void Node::SetOutputSlotLinked(int index) {
		mOutputSlots[index].isLinked = true;
	}

	void Node::SetOutputSlotBroken(int index) {
		mOutputSlots[index].isLinked = false;
	}

	void Node::SetPosition(float x, float y) {
		mPosition = Math::Vector2(x, y);
	}

	Node::Node(int id, NodeType nodeType, const std::string& label)
	: ImnodeObject(id) 
	, mNodeType(nodeType)
	, mLabel(label) {}
}