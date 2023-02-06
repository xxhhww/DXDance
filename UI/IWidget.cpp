#include "IWidget.h"
#include "imgui.h"

namespace UI {
	int64_t IWidget::smWidgetIDInc = 0;

	IWidget::IWidget() {
		mWidgetID = "##" + std::to_string(smWidgetIDInc++);
	}

	void IWidget::Destory() {
		mDestory = true;
	}

	void IWidget::SetParent(IWidgetContainer* parent) {
		mParent = parent;
	}

	void IWidget::Draw() {
		if (mEnable) {
			_Draw_Internal_Impl();
			if (!mLineBreak) {
				ImGui::SameLine();
			}
		}
	}
}