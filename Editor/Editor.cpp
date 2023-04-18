#include "Editor.h"
#include "MainMenuBar.h"
#include "ShaderEditor.h"
#include "AssetBrowser.h"
#include "SceneView.h"

#include "Core/ServiceLocator.h"

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
		mSceneView = &mCanvas.CreatePanel<SceneView>("Scene View");
		mSceneView->BindHandle(cpuHandle, gpuHandle);

		mainMenuBar.RegisterPanel(mShaderEditor);
		mainMenuBar.RegisterPanel(mAssetBrowser);
		mainMenuBar.RegisterPanel(mSceneView);

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
		// ������Ϊ�߼�

		// ͼ����Ⱦ
		mSceneView->Update(delta);
		mSceneView->Render(delta);
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