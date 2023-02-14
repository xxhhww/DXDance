#pragma once
#include "Context.h"

namespace App {
	class Editor {
	public:
		/*
		* �༭����ʼ��
		*/
		Editor(Context& context);

		/*
		* �༭������
		*/
		~Editor();

		/*
		* �༭�����º���
		*/
		void Update(float delta);

		/*
		* ���Ʊ༭�����еĿ������
		*/
		void DrawEditorPanels();
	private:
		Context& mContext;	// ����������
		UI::Canvas mCanvas;	// �༭������
	};
}