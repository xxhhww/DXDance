#include "Game/Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	Game::Application app("DXDance", hInstance, nCmdShow);
	return app.Run();
}