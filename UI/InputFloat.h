#pragma once
#include "IDataWidget.h"

namespace UI {
	class InputFloat : public IDataWidget<float> {
	public:
		InputFloat(
			float value,
			float step = 0.1f,
			float stepFast = 0.0f,
			const std::string& label = "",
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<float> editCompletedEvent;
	private:
		float mStep;
		float mStepFast;
		std::string mLabel;
		std::string mFormat;
	};
}