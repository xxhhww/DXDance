#pragma once
#include <array>
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {

	class Columns : public IWidget, public IWidgetContainer {
	public:
		/*
		* @param cols: ����
		* @param width: ÿ�еĿ��
		* @param autoArrange: �Ƿ��Զ�����
		* @param border: �Ƿ���ʾ�߽�
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