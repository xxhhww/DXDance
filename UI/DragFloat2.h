#pragma once
#include "IDataWidget.h"
#include "Math/Vector.h"

namespace UI {
	class DragFloat2 : public IDataWidget<Math::Vector2> {
	public:
		DragFloat2(
			const std::string& label,
			Math::Vector2 value = Math::Vector2{ 0.0f, 0.0f },
			float min = 0.0f,
			float max = 0.0f,
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector2> editCompletedEvent;
		std::string		label;
		float			min;
		float			max;
		float			speed;
		std::string		format;
	};

	class DragFloat2Split : public IDataWidget<Math::Vector2> {
	public:
		DragFloat2Split(
			const std::string& label,
			Math::Vector2 value = Math::Vector2{},
			Math::Vector2 min = Math::Vector2{},
			Math::Vector2 max = Math::Vector2{},
			float speed = 0.1f,
			const std::string& format = "%.3f");
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Vector2> editCompletedEvent;
		std::string		label;
		Math::Vector2	min;
		Math::Vector2	max;
		float			speed;
		std::string		format;
	};
}