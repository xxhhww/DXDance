#pragma once
#include "IInspectorItem.h"

namespace Core {

	class IComponent : public IInspectorItem {
	public:
		virtual ~IComponent() = default;
	};
}