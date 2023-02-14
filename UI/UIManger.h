#pragma once
#include "Canvas.h"
#include "Windows/Window.h"
#include "Setting.h"

namespace UI {
	class UIManger {
	public:
		/*
		* ImGUi��Imnodes�ĳ�ʼ���������ڴ�������D3D�����(�豸���������ѡ���������)
		*/
		UIManger(Windows::Window* window, UIStyle style);

		/*
		* ImGui��Imnodes������������(Canvas)��������Editor�����
		*/
		~UIManger();

		/*
		* Ӧ�ö�Ӧ��ImGui���
		*/
		void ApplyStyle(UIStyle style);

		/*
		* ����UIManger�Ļ�������Editor���������
		*/
		void SetCanvas(Canvas* canvas);

		/*
		* ���ƻ���
		*/
		void Darw();
	private:
		Canvas* mCanvas{ nullptr };
		UIStyle mStyle;
	};
}