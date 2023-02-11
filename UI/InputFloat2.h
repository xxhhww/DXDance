#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class InputFloat2 : public IDataWidget<Math::Vector2> {
	public:
		InputFloat2(
			Math::Vector2 value,
			float step = 0.1f,
			float stepFast = 0.0f,
			const std::string& label = "",
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector2> editCompletedEvent;
	private:
		float mStep;
		float mStepFast;
		std::string mLabel;
		std::string mFormat;
	};
}