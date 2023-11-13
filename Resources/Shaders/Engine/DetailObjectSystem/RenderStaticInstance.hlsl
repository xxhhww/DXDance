#ifndef _RenderStaticInstance__
#define _RenderStaticInstance__

#include "../Base/Light.hlsl"

struct PassData {
	
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

#endif