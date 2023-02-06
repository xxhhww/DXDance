#include "IPlane.h"

namespace UI {
	int64_t IPlane::smPlaneIDInc = 0;

	IPlane::IPlane() {
		mPlaneID = std::to_string(smPlaneIDInc++);
	}

	void IPlane::Draw() {
		if (mEnable) {
			_Draw_Internal_Impl();
		}
	}
}