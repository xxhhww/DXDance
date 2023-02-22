#pragma once
#include "IWidgetContainer.h"
#include "Math/Vector.h"

namespace UI {
	class Child : public IWidget, public IWidgetContainer {
	public:
		Child(const std::string& name, float widthScale = 0.0f, bool border = true);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		bool useBorder;
	private:
		std::string		mName;
		float			mWidthScale;
	};
}