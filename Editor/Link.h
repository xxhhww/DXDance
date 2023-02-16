#pragma once
#include "Tools/MemoryStream.h"

namespace App {
	class Link {
	public:
		Link(int id, int startPin, int endPin);
		inline ~Link() = default;

		void Draw();

		int StartNodeID()	const;
		int EndNodeID()		const;
		int StartPinIndex() const;
		int EndPinIndex()	const;

		void Serialize(Tool::OutputMemoryStream& blob) const;
		void Deserialize(Tool::InputMemoryStream& blob);
	public:
		int objectID{ -1 };
		int startPin{ -1 };
		int endPin	{ -1 };
	};
}