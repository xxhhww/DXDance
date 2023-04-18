#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Renderer {

	enum class CameraType {
		RenderCamera = 0,
		EditorCamera = 1
	};

	class Camera {
	public:
		// ������ռ�Ļ�����
		Math::Vector3 lookUp;
		Math::Vector3 right;
		Math::Vector3 up;

		CameraType cameraType;

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
		Camera();
	};

}