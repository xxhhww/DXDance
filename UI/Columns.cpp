#include "Columns.h"
#include "imgui.h"

namespace UI {
	/*
	* @param width: ÿ�еĿ��
	* @param autoArrange: �Ƿ��Զ�����
	* @param border: �Ƿ���ʾ�߽�
	*/
	Columns::Columns(int cols, float width, bool autoArrange, bool border)
	: cols(cols)
	, width(width) 
	, autoArrange(autoArrange)
	, useBorder(border) {
	}

	void Columns::_Draw_Internal_Impl() {
		int count = autoArrange ? ((int)ImGui::GetContentRegionAvail().x/ width) : cols;
		count = count < 1 ? 1 : count;

		ImGui::Columns(count, ("##" + mWidgetID).c_str(), useBorder);
		DoDestruction();

		int index = 0;
		for (auto it = mWidgets.begin(); it != mWidgets.end(); it++) {
			it->first->Draw();

			if (width != 0.0f) {
				// ImGui::SetColumnWidth(index,width);
			}

			ImGui::NextColumn();
			index++;
		}

		ImGui::Columns(1); // Necessary to not break the layout for following widget
	}
}