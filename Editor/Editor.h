#pragma once
#include "Context.h"

namespace App {
	class ShaderEditor;

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
		* �༭��������
		*/
		void PostUpdate(float delta);

		/*
		* ���Ʊ༭�����еĿ������
		*/
		void DrawEditorPanels();
	private:
		Context& mContext;	// ����������
		UI::Canvas mCanvas;	// �༭������
		ShaderEditor* mShaderEditor{ nullptr };
	};
}