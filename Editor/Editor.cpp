#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"

namespace App {
	/*
	* 编辑器初始化
	*/
	Editor::Editor(Context& context)
	: mContext(context) {

		auto& mainMenuBar = mCanvas.CreatePanel<MainMenuBar>();


		mShaderEditor = &mCanvas.CreatePanel<ShaderEditor>("ShaderEditor");

		mainMenuBar.RegisterPanel(mShaderEditor);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* 编辑器析构
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* 编辑器更新函数
	*/
	void Editor::Update(float delta) {
		// 处理编辑器输入
		mShaderEditor->HandleShortCut();
		// 处理Actor的行为逻辑

		// 图形渲染

		// 绘制画布
		DrawEditorPanels();
	}

	void Editor::PostUpdate(float delta) {
		mContext.inputManger->ClearStates();
	}
	/*
	* 绘制编辑器所有的控制面板
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}