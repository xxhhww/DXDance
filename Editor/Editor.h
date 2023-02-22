#pragma once
#include "Context.h"

namespace App {
	class ShaderEditor;
	class AssetBrowser;

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
		* �༭�����к���
		*/
		void Run();

		/*
		* �༭��Ԥ������
		*/
		void PreUpdate(float delta);

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
		AssetBrowser* mAssetBrowser{ nullptr };
	};
}