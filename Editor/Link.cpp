#include "Link.h"
#include "UI/imnodes.h"

namespace App {
	Link::Link(int id, int startPin, int endPin)
	: objectID(id)
	, startPin(startPin)
	, endPin(endPin) {}

	void Link::Draw() {
		ImNodes::Link(objectID, startPin, endPin);
	}

	int Link::StartNodeID() const {
		return startPin & 0x0000FFFF;
	}

	int Link::EndNodeID() const {
		return endPin & 0x0000FFFF;
	}

	int Link::StartPinIndex() const {
		return (startPin >> 16) & 0x000000FF;
	}

	int Link::EndPinIndex() const {
		return (endPin >> 16) & 0x000000FF;
	}

	void Link::Serialize(Tool::OutputMemoryStream& blob) const {
		blob.Write(objectID);
		blob.Write(startPin);
		blob.Write(endPin);
	}

	void Link::Deserialize(Tool::InputMemoryStream& blob) {
	}
}