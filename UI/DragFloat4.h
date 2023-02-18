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
		std::string		label;
		Math::Vector4	min;
		Math::Vector4	max;
		float			speed;
		std::string		format;
	};

	class DragFloat4Split : public IDataWidget<Math::Vector4> {
	public:
		DragFloat4Split(
			const std::string& label,
			Math::Vector4 value = Math::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f },
			Math::Vector4 min = Math::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f },
			Math::Vector4 max = Math::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f },
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector4> editCompletedEvent;
		std::string		label;
		Math::Vector4	min;
		Math::Vector4	max;
		float			speed;
		std::string		format;
	};
}