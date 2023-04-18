#include "SceneView.h"
#include "UI/Image.h"

namespace App {
	SceneView::SceneView(
		const std::string& title,
		bool opend,
		const UI::PanelWindowSettings& panelSetting)
	: UI::PanelWindow(title, opend, panelSetting) 
	, mFinalOutputRect(1920.0f, 1080.0f)
	, mRenderEngine(nullptr, mFinalOutputRect.x, mFinalOutputRect.y) {
		mBackImage = &CreateWidget<UI::Image>(0u, mFinalOutputRect);
	}

	void SceneView::BindHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
		mRenderEngine.BindFinalOuputSRV(cpuHandle);
		mBackImage->textureID = gpuHandle.ptr;
	}

	void SceneView::Update(float dt) {
		mAvailableSize = GetAvailableSize();
		mBackImage->size = mAvailableSize;
	}

	void SceneView::Render(float dt) {
		mRenderEngine.Update(dt);
		mRenderEngine.Render();
	}

	Math::Vector2 SceneView::GetAvailableSize() const {
		Math::Vector2 result = GetSize() - Math::Vector2{ 0.0f, 25.0f }; // 25 == title bar height
		return result;
	}

}