#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class SliderFloat2 : public IDataWidget<Math::Vector2> {
	public:
		SliderFloat2(
			const std::string& label,
			Math::Vector2 value,
			float min = 0.0f,
			float max = 0.0f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector2> editCompletedEvent;
	private:
		std::string mLabel;
		float mMin;
		float mMax;
		std::string mFormat;
	};
}