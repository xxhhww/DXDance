#ifndef _Light__
#define _Light__

#include "BRDFHelper.hlsl"
#include "../Math/MathCommon.hlsl"

#define MAX_NUM_SUN_SHADOW_CASCADES 4

struct Light {
	float4 position;	// sun direction + sunDiskArea
	float3 color;		// sun illuminance
	float  intensity;	
	float3 radiance;	// color * instensity
	uint   type;
};

#endif