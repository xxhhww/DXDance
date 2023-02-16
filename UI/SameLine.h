#pragma once
#include "IWidget.h"

namespace UI {
	class SameLine : public IWidget {
	protected:
		void _Draw_Internal_Impl() override;
	};
}
