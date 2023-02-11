#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class SliderFloat3 : public IDataWidget<Math::Vector3> {
	public:
		SliderFloat3(
			const std::string& label,
			Math::Vector3 value,
			float min = 0.0f,
			float max = 0.0f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector3> editCompletedEvent;
	private:
		std::string mLabel;
		float mMin;
		float mMax;
		std::string mFormat;
	};
}