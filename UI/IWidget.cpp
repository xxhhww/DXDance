#include "IWidget.h"
#include "imgui.h"

namespace UI {
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
		}
	}
}