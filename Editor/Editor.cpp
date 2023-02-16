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


		auto& shaderEditor = mCanvas.CreatePanel<ShaderEditor>("ShaderEditor");

		mainMenuBar.RegisterPanel(&shaderEditor);

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
		// 更新编辑器数据
		// 处理Actor的行为逻辑
		// 渲染数据
		// 绘制画布
		DrawEditorPanels();
	}

	/*
	* 绘制编辑器所有的控制面板
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}