#pragma once
#include "MemoryStream.h"

namespace Tool {
	class ISerializable {
	public:
		virtual ~ISerializable() = default;
		virtual void Serialize(OutputMemoryStream& blob) const	= 0;
		virtual void Deserialize(const InputMemoryStream& blob) = 0;
	};
}