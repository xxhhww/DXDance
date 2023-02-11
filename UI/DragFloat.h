#pragma once
#include "IDataWidget.h"

namespace UI {
	class DragFloat : public IDataWidget<float> {
	public:
		DragFloat(
			const std::string& label,
			float value,
			float min = 0.0f,
			float max = 0.0f,
			float speed = 0.1f, 
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<float> editCompletedEvent;
	private:
		std::string mLabel;
		float mMin;
		float mMax;
		float mSpeed;
		std::string mFormat;
	};
}