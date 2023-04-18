#pragma once
#include "UI/PanelWindow.h"
#include "Renderer/RenderEngine.h"

namespace UI {
	class Image;
}

namespace App {

	class SceneView : public UI::PanelWindow {
	public:
		SceneView(
			const std::string& title = "SceneView",
			bool opend = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);
		~SceneView() = default;

		void BindHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

		void Update(float dt);

		void Render(float dt);

	private:
		Math::Vector2 GetAvailableSize() const;

	private:
		Math::Vector2 mFinalOutputRect{ 0.0f, 0.0f };
		Math::Vector2 mAvailableSize{ 0.0f, 0.0f };
		Renderer::RenderEngine mRenderEngine;
		UI::Image* mBackImage{ nullptr };
	};

}