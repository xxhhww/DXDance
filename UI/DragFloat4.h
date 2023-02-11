#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class DragFloat4 : public IDataWidget<Math::Vector4> {
	public:
		DragFloat4(
			const std::string& label,
			Math::Vector4 value,
			float min = 0.0f,
			float max = 0.0f,
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector4> editCompletedEvent;
	private:
		std::string mLabel;
		float mMin;
		float mMax;
		float mSpeed;
		std::string mFormat;
	};
}