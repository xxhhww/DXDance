#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <array>

namespace ECS {

	enum class CameraType {
		RenderCamera = 0,
		EditorCamera = 1
	};

	class Camera : public ECS::IComponent {
	public:
		struct Jitter {
			Math::Matrix4 jitterMatrix;
			Math::Vector2 uvJitter;
		};

	private:
		inline static std::array<Math::Vector2, 16> smJitterHaltonSamples = {
			Math::Vector2{ 0.000000 , -0.166667 },
			Math::Vector2{ -0.250000,  0.166667 },
			Math::Vector2{ 0.250000 , -0.388889 },
			Math::Vector2{ -0.375000, -0.055556 },
			Math::Vector2{ 0.125000 ,  0.277778 },
			Math::Vector2{ -0.125000, -0.277778 },
			Math::Vector2{ 0.375000 ,  0.055556 },
			Math::Vector2{ -0.437500,  0.388889 },
			Math::Vector2{ 0.062500 , -0.462963 },
			Math::Vector2{ -0.187500, -0.129630 },
			Math::Vector2{ 0.312500 ,  0.203704 },
			Math::Vector2{ -0.312500, -0.351852 },
			Math::Vector2{ 0.187500 , -0.018519 },
			Math::Vector2{ -0.062500,  0.314815 },
			Math::Vector2{ 0.437500 , -0.240741 },
			Math::Vector2{ -0.468750,  0.092593 },
		};

	public:
		// 摄像机空间的基向量
		Math::Vector3 lookUp;
		Math::Vector3 right;
		Math::Vector3 up;

		CameraType cameraType;
		bool mainCamera;			// 一次只能有一个RenderCamera被设置为mainCamera

		float rotationSpeed;		// 旋转速度
		float translationSpeed;		// 行进速度

		struct Frustum {
			float nearZ;
			float farZ;
			float aspect;
			float fovY;
		} frustum;					// 平截头体参数

		struct FrustumPlane {
			float planeDistance;
			Math::Vector3 planeNormal;
		} frustumPlaneArray[6];		// 描述平截头体的六个平面

		Math::Matrix4 viewMatrix;		// 视图变换矩阵
		Math::Matrix4 projMatrix;		// 投影变换矩阵
		Math::Matrix4 viewProjMatrix;	// vp矩阵

	public:
		Camera()
		: lookUp(0.0f, 0.0f, 1.0f)
		, right(1.0f, 0.0f, 0.0f)
		, up(0.0f, 1.0f, 0.0f)
		, cameraType(CameraType::RenderCamera)
		, mainCamera(false)
		, rotationSpeed(0.05f)
		, translationSpeed(10.0f) {

			frustum.nearZ = 1.0f;
			frustum.farZ = 1000.0f;
			frustum.aspect = 979.0f / 635.0f;
			frustum.fovY = 0.5f * DirectX::XM_PI;

			viewMatrix = Math::Matrix4();
			projMatrix = Math::Matrix4();
		}

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

		void OnInspector(UI::IWidgetContainer* container) override {
			auto* group = &container->CreateWidget<UI::GroupCollapsable>("Camera");

			auto& nearZItem = group->CreateWidget<UI::SliderFloat>("nearZ", 0.0f, 0.0f, 10.0f);
			nearZItem.dataGatherer = [this]() -> float {
				return frustum.nearZ;
			};
			nearZItem.dataProvider = [this](float newValue) {
				frustum.nearZ = newValue;
			};

			auto& farZItem = group->CreateWidget<UI::SliderFloat>("farZ", 0.0f, 100.0f, 1000.0f);
			farZItem.dataGatherer = [this]() -> float {
				return frustum.farZ;
			};
			farZItem.dataProvider = [this](float newValue) {
				frustum.farZ = newValue;
			};

			auto& fovItem = group->CreateWidget<UI::SliderFloat>("Fov", 0.0f, 0.0f, 90.0f);
			fovItem.dataGatherer = [this]() -> float {
				return frustum.fovY / DirectX::XM_PI * 180.0f;
			};
			fovItem.dataProvider = [this](float newValue) {
				frustum.fovY = newValue / 180.0f * DirectX::XM_PI;
			};
		}

	};

}