#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class DragFloat3 : public IDataWidget<Math::Vector3> {
	public:
		DragFloat3(
			const std::string& label,
			Math::Vector3 value = Math::Vector3{ 0.0f, 0.0f, 0.0f },
			float min = 0.0f,
			float max = 0.0f,
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector3> editCompletedEvent;
		std::string		label;
		float			min;
		float			max;
		float			speed;
		std::string		format;
	};

	class DragFloat3Split : public IDataWidget<Math::Vector3> {
	public:
		DragFloat3Split(
			const std::string& label,
			Math::Vector3 value = Math::Vector3{},
			Math::Vector3 min = Math::Vector3{},
			Math::Vector3 max = Math::Vector3{},
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector3> editCompletedEvent;
		std::string		label;
		Math::Vector3	min;
		Math::Vector3	max;
		float			speed;
		std::string		format;
	};
}