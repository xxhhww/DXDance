#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Renderer {

	enum class CameraType {
		RenderCamera = 0,
		EditorCamera = 1
	};

	class Camera : public ECS::IComponent {
	public:
		// 摄像机空间的基向量
		Math::Vector3 lookUp;
		Math::Vector3 right;
		Math::Vector3 up;

		CameraType cameraType;
		bool mainCamera;			// 一次只能有一个RenderCamera被设置为mainCamera

		float rotationSpeed;		// 旋转速度
		float translationSpeed;		// 行进速度

		float lookUpMovingDir;		// 摄像机是否在沿着LookUp方向前进(取值: -1;0;1)
		float rightMovingDir;		// 摄像机是否在沿着Right方向前进 (取值: -1;0;1)

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

	public:
		Camera()
			: lookUp(0.0f, 0.0f, 1.0f)
			, right(1.0f, 0.0f, 0.0f)
			, up(0.0f, 1.0f, 0.0f)
			, cameraType(CameraType::RenderCamera)
			, mainCamera(false)
			, rotationSpeed(0.004f)
			, translationSpeed(0.05f)
			, lookUpMovingDir(0.0f)
			, rightMovingDir(0.0f) {

			frustum.nearZ = 1.0f;
			frustum.farZ = 1000.0f;
			frustum.aspect = 1.0f;
			frustum.fovY = 0.25f * DirectX::XM_PI;

			viewMatrix = Math::Matrix4();
			projMatrix = Math::Matrix4();
		}

		void SerializeJson(Tool::JsonWriter& writer) const override {

		}

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

	};

}