#include "CCamera.h"

namespace Renderer {
	Camera::Camera()
	: lookUp(0.0f, 0.0f, 1.0f)
	, right(1.0f, 0.0f, 0.0f)
	, up(0.0f, 1.0f, 0.0f)
	, cameraType(CameraType::RenderCamera)
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
}