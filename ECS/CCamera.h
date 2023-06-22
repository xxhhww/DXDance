#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"


namespace ECS {

	enum class CameraType {
		RenderCamera = 0,
		EditorCamera = 1
	};

	class Camera : public ECS::IComponent {
	public:
		using FStop = float;
		using ISO = float;
		using EV = float;

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

		Math::Matrix4 viewMatrix;	// 视图变换矩阵
		Math::Matrix4 projMatrix;	// 投影变换矩阵

		/// Relative aperture. Controls how wide the aperture is opened. Impacts the depth of field.
		FStop mLenseAperture;

		/// Sensor sensitivity/gain. Controls how photons are counted / quantized on the digital sensor.
		ISO mFilmSpeed;

		/// Controls how long the aperture is opened. Impacts the motion blur.
		float mShutterTime;

	public:
		inline EV GetExposureValue100() const {
			// EV number is defined as:
			// 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
			// This gives
			// EV_s = log2 (N^2 / t)
			// EV_100 + log2 (S /100) = log2 (N^2 / t)
			// EV_100 = log2 (N^2 / t) - log2 (S /100)
			// EV_100 = log2 (N^2 / t . 100 / S)
			return std::log2((mLenseAperture * mLenseAperture) / mShutterTime * 100.0 / mFilmSpeed);
		}

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
			frustum.aspect = 1920.0f / 1080.0f;
			frustum.fovY = 0.5f * DirectX::XM_PI;

			viewMatrix = Math::Matrix4();
			projMatrix = Math::Matrix4();

			mLenseAperture = 1.6f;
			mFilmSpeed = 100.0f;
			mShutterTime = 1.0f / 125.0f;
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