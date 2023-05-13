#pragma once
#include "Context.h"

namespace App {
	class ShaderEditor;
	class AssetBrowser;
	class Hierarchy;
	class SceneView;

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
		* �༭�����º���
		*/
		void Update(float delta);

		/*
		* ���Ʊ༭�����еĿ������
		*/
		void DrawEditorPanels();
	private:
		/*
		* �༭��Ԥ������
		*/
		void PreUpdate(float delta);

		/*
		* �༭��������
		*/
		void PostUpdate(float delta);

		/*
		* ���±༭�����
		*/
		void UpdateEditorPanels(float delta);

		/*
		* ������ͼ���
		*/
		void UpdateAndRenderViewPanels(float delta);

	private:
		Context& mContext;	// ����������
		UI::Canvas mCanvas;	// �༭������
		ShaderEditor* mShaderEditor{ nullptr };
		AssetBrowser* mAssetBrowser{ nullptr };
		Hierarchy*    mHierarchy{ nullptr };
		SceneView*    mSceneView{ nullptr };
	};
}