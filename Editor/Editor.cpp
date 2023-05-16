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
	* �༭����ʼ��
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
		
		// Hierarchy�ĳ�ʼ���������SceneView�ĳ�ʼ��֮ǰ��SceneView��ʼ��ʱ�����Actor�Ĵ����������¼�����Щ�¼��Ļص������ӵ�Hierarchy��
		mHierarchy = &mCanvas.CreatePanel<Hierarchy>("Hierarchy");

		mSceneView = &mCanvas.CreatePanel<SceneView>("Scene View");
		mSceneView->BindHandle(cpuHandle, gpuHandle);

		// Inspector�ĳ�ʼ���������SceneView�ĳ�ʼ��֮��Inspector��ʼ��ʱ��ʹ��MainCamera��ΪĬ�ϵĽ���
		mInspector = &mCanvas.CreatePanel<Inspector>("Inspector");

		mainMenuBar.RegisterPanel(mShaderEditor);
		mainMenuBar.RegisterPanel(mAssetBrowser);
		mainMenuBar.RegisterPanel(mHierarchy);
		mainMenuBar.RegisterPanel(mSceneView);
		mainMenuBar.RegisterPanel(mInspector);

		mContext.uiManger->SetCanvas(&mCanvas);
	}

	/*
	* �༭������
	*/
	Editor::~Editor() {
		// �ó����е�Actor��ǰ��Hierarchy�ͷ�
		CORESERVICE(Core::SceneManger).UnLoadCurrentScene();
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
	* �༭�����º���
	*/
	void Editor::Update(float delta) {
		UpdateEditorPanels(delta);
		UpdateAndRenderViewPanels(delta);
		// ���ƻ���
		DrawEditorPanels();
	}

	/*
	* ���Ʊ༭�����еĿ������
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