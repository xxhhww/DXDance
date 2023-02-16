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


		auto& shaderEditor = mCanvas.CreatePanel<ShaderEditor>("ShaderEditor");

		mainMenuBar.RegisterPanel(&shaderEditor);

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
		// ���±༭������
		// ����Actor����Ϊ�߼�
		// ��Ⱦ����
		// ���ƻ���
		DrawEditorPanels();
	}

	/*
	* ���Ʊ༭�����еĿ������
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}