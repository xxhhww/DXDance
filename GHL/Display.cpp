#include "Display.h"

namespace GHL {
	Display::Display(const DXGI_OUTPUT_DESC1& output) {
        redPrimary = { output.RedPrimary[0], output.RedPrimary[1] };
        greenPrimary = { output.GreenPrimary[0], output.GreenPrimary[1] };
        bluePrimary = { output.BluePrimary[0], output.BluePrimary[1] };
        whitePoint = { output.WhitePoint[0], output.WhitePoint[1] };
        minLuminance = output.MinLuminance;
        maxLuminance = output.MaxLuminance;
        maxFullFrameLuminance = output.MaxFullFrameLuminance;
        supportHDR = false;
        rectOrigin = { (float)output.DesktopCoordinates.left, (float)output.DesktopCoordinates.top };
        rectSize = {
            (float)output.DesktopCoordinates.right - (float)output.DesktopCoordinates.left,
            (float)output.DesktopCoordinates.bottom - (float)output.DesktopCoordinates.top
        };
	}

}