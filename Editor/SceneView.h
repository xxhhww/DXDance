#pragma once
#include "UI/PanelWindow.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

#include "Renderer/RenderEngine.h"
#include "Renderer/Texture.h"

namespace UI {
	class Image;
}

namespace Core {
	class Scene;
	class SceneManger;
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

		void LoadNewScene(const std::string& path);

		void Update(float dt);

		void Render(float dt);

	private:
		Math::Vector2 GetAvailableSize() const;

		/*
		* 处理用户拾取
		*/
		void HandleActorPicking();

		/*
		* 将Actor与坐标轴渲染到纹理中
		*/
		void RenderSceneForActorPicking();

		/*
		* 渲染编辑器常用的小组件
		*/
		void RenderGizmo();

	private:
		Math::Vector2 mAvailableSize{ 0.0f, 0.0f };
		Renderer::RenderEngine* mRenderEngine{ nullptr };
		UI::Image* mBackImage{ nullptr };

		Core::SceneManger* mSceneManger{ nullptr };
		Core::Scene*	   mCurrentScene{ nullptr };
		ECS::Camera*	   mEditorCamera;
		ECS::Transform*    mEditorTransform;

		std::unique_ptr<Renderer::Texture>  mPickingTexture;
	};

}