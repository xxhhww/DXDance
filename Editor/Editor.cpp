#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"
#include "AssetBrowser.h"

namespace App {
	/*
	* 编辑器初始化
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
	* 编辑器析构
	*/
	Editor::~Editor() {
		mCanvas.DeleteAllPanels();
	}

	/*
	* 编辑器运行函数
	*/
	void Editor::Run() {
		float delta = mContext.clock->GetDeltaTime();

		PreUpdate(delta);
		Update(delta);
		PostUpdate(delta);

		mContext.clock->Update();
	}

	/*
	* 编辑器预处理函数
	*/
	void Editor::PreUpdate(float delta) {
		mContext.inputManger->PreUpdate(delta);
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
		mContext.inputManger->PostUpdate();
	}
	/*
	* 绘制编辑器所有的控制面板
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}
}