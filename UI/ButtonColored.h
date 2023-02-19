#pragma once
#include "IWidget.h"
#include "Tools/Event.h"
#include "Math/Vector.h"
#include "Math/Color.h"

namespace UI {
	class ButtonColored : public IWidget {
	public:
		ButtonColored(
			const std::string& label, 
			const Math::Color& color = Math::Color{}, 
			const Math::Vector2& size = Math::Vector2{}, 
			bool enableAlpha = false);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<>	clickedEvent;
		std::string		label;
		Math::Color		color;
		Math::Vector2	size;
		bool			enableAlpha;
	};
}