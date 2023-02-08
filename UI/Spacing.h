#pragma once
#include "IWidget.h"

namespace UI {
	class Spacing : public IWidget {
	public:
		Spacing(uint32_t spacingNums);
	protected:
		void _Draw_Internal_Impl() override;
	private:
		uint32_t mSpacings;
	};
}