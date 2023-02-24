#pragma once
#include <array>
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {

	class Columns : public IWidget, public IWidgetContainer {
	public:
		/*
		* @param cols: 列数
		* @param width: 每列的宽度
		* @param autoArrange: 是否自动排列
		* @param border: 是否显示边界
		*/
		Columns(int cols = 1, float width = 0.0f, bool autoArrange = true, bool border = false);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		int		cols{ 1 };
		float	width{ 0.0f };
		bool	autoArrange{ true };
		bool	useBorder{ false };
	};
}