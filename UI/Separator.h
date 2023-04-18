#pragma once
#include "IWidget.h"

namespace UI {

	class Separator : public IWidget {
	protected:
		void _Draw_Internal_Impl() override;
	};

}