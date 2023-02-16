#pragma once
#include <array>
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {
	template<int TSize>
	class Columns : public IWidget, public IWidgetContainer {
	public:
		Columns(bool border = true);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		bool useBorder;
		std::array<float, TSize> widths;	// 每列的宽度
	};
}

#include "Columns.inl"