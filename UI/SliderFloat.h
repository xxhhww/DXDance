#pragma once
#include "IDataWidget.h"

namespace UI {
	class SliderFloat : public IDataWidget<float> {
	public:
		SliderFloat(
			const std::string& label,
			float value,
			float min = 0.0f,
			float max = 0.0f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<float> editCompletedEvent;
	private:
		std::string mLabel;
		float mMin;
		float mMax;
		std::string mFormat;
	};
}