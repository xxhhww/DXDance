#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class InputFloat3 : public IDataWidget<Math::Vector3> {
	public:
		InputFloat3(
			Math::Vector3 value,
			float step = 0.1f,
			float stepFast = 0.0f,
			const std::string& label = "",
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector3> editCompletedEvent;
	private:
		float mStep;
		float mStepFast;
		std::string mLabel;
		std::string mFormat;
	};
}