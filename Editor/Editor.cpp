#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"
#include "AssetBrowser.h"

namespace App {
	/*
	* �༭����ʼ��
	*/
	Editor::Editor(Context& context)
	: mContext(context) {

		auto& mainMenuBar = mCanvas.CreatePanel<MainMenuBar>("Main Menu Bar");


		mShaderEditor = &mCanvas.CreatePanel<ShaderEditor>("Shader Editor");
		mAssetBrowser = &mCanvas.CreatePanel<AssetBrowser>("Asset Browser", mContext.projectEnginePath, mContext.projectAssetPath);

		mainMenuBar.RegisterPanel(mShaderEditor);
		mainMenuBar.RegisterPanel(mAssetBrowser);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* �༭������
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* �༭�����к���
	*/
	void Editor::Run() {
		float delta = mContext.clock->GetDeltaTime();

		PreUpdate(delta);
		Update(delta);
		PostUpdate(delta);

		mContext.clock->Update();
	}

	/*
	* �༭��Ԥ������
	*/
	void Editor::PreUpdate(float delta) {
		mContext.inputManger->PreUpdate(delta);
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
		mContext.inputManger->PostUpdate();
	}
	/*
	* ���Ʊ༭�����еĿ������
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}