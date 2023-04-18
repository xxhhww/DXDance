#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"
#include "AssetBrowser.h"
#include "SceneView.h"

#include "Core/ServiceLocator.h"

namespace App {
	/*
	* 编辑器初始化
	*/
	Editor::Editor(Context& context)
	: mContext(context) {
		auto* srvHeap = CORESERVICE(UI::UIManger).GetSRDescriptorHeap();
		auto incSize = CORESERVICE(UI::UIManger).GetSRVIncrementSize();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

		cpuHandle.ptr += incSize * 1u;
		gpuHandle.ptr += incSize * 1u;

		auto& mainMenuBar = mCanvas.CreatePanel<MainMenuBar>("Main Menu Bar");

		mShaderEditor = &mCanvas.CreatePanel<ShaderEditor>("Shader Editor");
		mAssetBrowser = &mCanvas.CreatePanel<AssetBrowser>("Asset Browser", mContext.projectEnginePath, mContext.projectAssetPath);
		mSceneView = &mCanvas.CreatePanel<SceneView>("Scene View");
		mSceneView->BindHandle(cpuHandle, gpuHandle);

		mainMenuBar.RegisterPanel(mShaderEditor);
		mainMenuBar.RegisterPanel(mAssetBrowser);
		mainMenuBar.RegisterPanel(mSceneView);

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
		// 处理行为逻辑

		// 图形渲染
		mSceneView->Update(delta);
		mSceneView->Render(delta);
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