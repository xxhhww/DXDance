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
		// 摄像机空间的基向量
		Math::Vector3 lookUp;
		Math::Vector3 right;
		Math::Vector3 up;

		CameraType cameraType;

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
		Camera();
	};

}