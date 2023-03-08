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
        rectOrigin = { output.DesktopCoordinates.left, output.DesktopCoordinates.top };
        rectSize = {
            output.DesktopCoordinates.right - output.DesktopCoordinates.left,
            output.DesktopCoordinates.bottom - output.DesktopCoordinates.top
        };
	}

}