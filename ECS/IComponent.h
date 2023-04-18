#pragma once
#include "Tools/ISerializable.h"

namespace ECS {

	class IComponent : public Tool::ISerializable {
	public:
		virtual ~IComponent() = default;
	};

}