#pragma once
#include "Columns.h"

namespace UI {
	template<int TSize>
	Columns<TSize>::Columns(bool border)
	: useBorder(border) {
		widths.fill(0.0f);
	}

	template<int TSize>
	void Columns<TSize>::_Draw_Internal_Impl() {
		ImGui::Columns(TSize, ("##" + mWidgetID).c_str(), false);
		DoDestruction();

		int counter = 0;
		for (auto it = mWidgets.begin(); it != mWidgets.end(); it++) {
			it->first->Draw();

			if (widths.at(counter) != 0.0f) {
				ImGui::SetColumnWidth(counter, widths.at(counter));
			}

			ImGui::NextColumn();
			++counter;
		}

		ImGui::Columns(1); // Necessary to not break the layout for following widget
	}
}