#include "Game/Application.h"
#include "Math/BoundingBox.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	/*
	Math::BoundingBox test;
	test.minPosition = Math::Vector3{ -1.0f, -1.0f, -1.0f };
	test.maxPosition = Math::Vector3{ 1.0f, 1.0f, 1.0f };

	Math::Matrix4 transform = Math::Vector3{ 0.0f, DirectX::XM_PIDIV4, 0.0f }.RotationMatrix();
	
	const auto res = test.transformBy(transform);
	*/

	Game::Application app("DXDance", hInstance, nCmdShow);
	return app.Run();
}