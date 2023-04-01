#include <Windows.h>
#include "DebugLayer.h"

using namespace GHL;

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    EnableDebugLayer();
}