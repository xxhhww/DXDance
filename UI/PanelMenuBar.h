#pragma once
#include "IPanel.h"

namespace UI {
	class PanelMenuBar : public IPanel {
	public:
	protected:
		void _Draw_Internal_Impl() override;
	};
}