#include "Columns.h"
#include "imgui.h"

namespace UI {
	Columns::Columns(bool border)
		: useBorder(border) {}

	void Column::_Draw_Internal_Impl() {
		ImGui::Columns(mCount, ("##" + mWidgetID).c_str(), useBorder);

	}
}