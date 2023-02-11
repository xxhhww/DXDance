#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class InputFloat4 : public IDataWidget<Math::Vector4> {
	public:
		InputFloat4(
			Math::Vector4 value,
			float step = 0.1f,
			float stepFast = 0.0f,
			const std::string& label = "",
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector4> editCompletedEvent;
	private:
		float mStep;
		float mStepFast;
		std::string mLabel;
		std::string mFormat;
	};
}