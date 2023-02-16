#include "Pin.h"
#include <unordered_map>
#include "UI/imnodes.h"
#include "Node.h"

namespace App {
	// Helper Function

	// 获取类型提示
	static std::unordered_map<PinType, std::string> sTypeHintingMap = {
		{ PinType::Bool			, {"(b)"}		},
		{ PinType::Float		, {"(1)"}		},
		{ PinType::Float2		, {"(2)"}		},
		{ PinType::Float3		, {"(3)"}		},
		{ PinType::Float4		, {"(4)"}		},
		{ PinType::SamplerState	, {"(ss)"}		},
		{ PinType::Texture2D	, {"(tex2d)"}	},
	};
	std::string GetTypeHinting(PinType type) {
		return sTypeHintingMap.at(type);
	}
}

namespace App {
	Pin::Pin(Node* node, int id, bool output, PinType slotType, const std::string& label)
	: ownNode(node) 
	, objectID(id)
	, isOutput(output) 
	, type(slotType) 
	, mLabel(label) {}

	void Pin::_Draw_Internal_Impl() {
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
	}
}