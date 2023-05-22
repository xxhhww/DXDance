#pragma once
#include "UI/PanelWindow.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

#include "Renderer/RenderEngine.h"
#include "Renderer/Texture.h"

#include "Math/Matrix.h"
#include "Math/Vector.h"

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
		* �����û�ʰȡ
		*/
		void HandleActorPicking();

		/*
		* ��Actor����������Ⱦ��������
		*/
		void RenderSceneForActorPicking();

		/*
		* ע��༭����ȾPass
		*/
		void RegisterEditorRenderPass();

		/*
		* Create D3DObject For Picking
		*/
		void CreateD3DObjectForPicking();

	private:
		Math::Vector2 mAvailableSize{ 0.0f, 0.0f };
		Renderer::RenderEngine* mRenderEngine{ nullptr };
		UI::Image* mBackImage{ nullptr };

		Core::SceneManger* mSceneManger{ nullptr };
		Core::Scene*	   mCurrentScene{ nullptr };
		ECS::Camera*	   mEditorCamera;
		ECS::Transform*    mEditorTransform;

		struct AxisPassData {
		public:
			Math::Matrix4 modelMatrix;
			Math::Matrix4 viewMatrix;
			Math::Matrix4 projMatrix;
			Math::Vector3 viewPos;
			int           highlightedAxis{ 3 }; // �����ѡ�еĿ�����
		} mAxisPassData;

		std::unique_ptr<Renderer::RingFrameTracker>      mPickingFrameTracker;
		std::unique_ptr<GHL::Fence>                      mPickingFrameFence;
		std::unique_ptr<Renderer::LinearBufferAllocator> mPickingLinearBufferAllocator;

		std::unique_ptr<GHL::CommandAllocator> mPickingCommandListAllocator;
		std::unique_ptr<GHL::CommandList>      mPickingCommandList;

		std::unique_ptr<Renderer::Texture>     mPickingRenderTarget;
		std::unique_ptr<GHL::DescriptorHeap>   mPickingRTDescriptorHeap;
		GHL::DescriptorHandle                  mPickingRTDescriptor;

		std::unique_ptr<Renderer::Buffer>      mPickingReadback;
	};

}