#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"

namespace App {
	/*
	* �༭����ʼ��
	*/
	Editor::Editor(Context& context)
	: mContext(context) {

		auto& mainMenuBar = mCanvas.CreatePanel<MainMenuBar>();


		mShaderEditor = &mCanvas.CreatePanel<ShaderEditor>("ShaderEditor");

		mainMenuBar.RegisterPanel(mShaderEditor);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* �༭������
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* �༭�����º���
	*/
	void Editor::Update(float delta) {
		// ����༭������
		mShaderEditor->HandleShortCut();
		// ����Actor����Ϊ�߼�

		// ͼ����Ⱦ

		// ���ƻ���
		DrawEditorPanels();
	}

	void Editor::PostUpdate(float delta) {
		mContext.inputManger->ClearStates();
	}
	/*
	* ���Ʊ༭�����еĿ������
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}