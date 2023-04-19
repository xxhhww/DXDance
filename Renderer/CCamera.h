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
		// ������ռ�Ļ�����
		Math::Vector3 lookUp;
		Math::Vector3 right;
		Math::Vector3 up;

		CameraType cameraType;
		bool mainCamera;			// һ��ֻ����һ��RenderCamera������ΪmainCamera

		float rotationSpeed;		// ��ת�ٶ�
		float translationSpeed;		// �н��ٶ�

		float lookUpMovingDir;		// ������Ƿ�������LookUp����ǰ��(ȡֵ: -1;0;1)
		float rightMovingDir;		// ������Ƿ�������Right����ǰ�� (ȡֵ: -1;0;1)

		struct Frustum {
			float nearZ;
			float farZ;
			float aspect;
			float fovY;
		} frustum;					// ƽ��ͷ�����

		struct FrustumPlane {
			float planeDistance;
			Math::Vector3 planeNormal;
		} frustumPlaneArray[6];		// ����ƽ��ͷ�������ƽ��

		Math::Matrix4 viewMatrix;	// ��ͼ�任����
		Math::Matrix4 projMatrix;	// ͶӰ�任����

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
		* �����л�Json����
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override {

		}

	};

}