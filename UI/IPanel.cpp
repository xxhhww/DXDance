#include "IPanel.h"

namespace UI {
	int64_t IPanel::smPlaneIDInc = 0;

	IPanel::IPanel() {
		mPlaneID = "##" + std::to_string(smPlaneIDInc++);
	}

	void IPanel::Draw() {
		if (mEnable) {
			_Draw_Internal_Impl();
		}
	}
}