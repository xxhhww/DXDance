#pragma once
#include "IWidget.h"
#include "Tools/Event.h"
#include "Math/Vector.h"

namespace UI {
	class Button : public IWidget {
	public:
		Button(const std::string& label, const Math::Vector2& size = Math::Vector2{}, bool disable = false);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<>	clickedEvent;
		Tool::Event<>	doubleClickedEvent;
		std::string		label;
		Math::Vector2	size;
		bool			disable;
	};
}