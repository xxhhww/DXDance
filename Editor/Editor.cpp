#include "Editor.h"

#include "Editor/MainMenuBar.h"
#include "Editor/ShaderEditor.h"
#include "Editor/AssetBrowser.h"
#include "Editor/Hierarchy.h"
#include "Editor/SceneView.h"
#include "Editor/Inspector.h"

#include "Core/ServiceLocator.h"
#include "Core/SceneManger.h"

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
		
		// Hierarchy的初始化必须放在SceneView的初始化之前，SceneView初始化时会产生Actor的创建、析构事件，这些事件的回调会链接到Hierarchy上
		mHierarchy = &mCanvas.CreatePanel<Hierarchy>("Hierarchy");

		mSceneView = &mCanvas.CreatePanel<SceneView>("Scene View");
		mSceneView->BindHandle(cpuHandle, gpuHandle);

		// Inspector的初始化必须放在SceneView的初始化之后，Inspector初始化时会使用MainCamera作为默认的焦点
		mInspector = &mCanvas.CreatePanel<Inspector>("Inspector");

		mainMenuBar.RegisterPanel(mShaderEditor);
		mainMenuBar.RegisterPanel(mAssetBrowser);
		mainMenuBar.RegisterPanel(mHierarchy);
		mainMenuBar.RegisterPanel(mSceneView);
		mainMenuBar.RegisterPanel(mInspector);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* 编辑器析构
	*/
	Editor::~Editor() {
		// 让场景中的Actor提前于Hierarchy释放
		CORESERVICE(Core::SceneManger).UnLoadCurrentScene();
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
	* 编辑器更新函数
	*/
	void Editor::Update(float delta) {
		UpdateEditorPanels(delta);
		UpdateAndRenderViewPanels(delta);
		// 绘制画布
		DrawEditorPanels();
	}

	/*
	* 绘制编辑器所有的控制面板
	*/
	void Editor::DrawEditorPanels() {
		mContext.uiManger->Darw();
	}

	void Editor::PreUpdate(float delta) {
		mContext.inputManger->PreUpdate(delta);
	}

	void Editor::PostUpdate(float delta) {
		mContext.inputManger->PostUpdate();
	}

	void Editor::UpdateEditorPanels(float delta) {

	}

	void Editor::UpdateAndRenderViewPanels(float delta) {
		if (mSceneView->IsOpened()) {
			mSceneView->Update(delta);
			mSceneView->Render(delta);
		}
	}

}