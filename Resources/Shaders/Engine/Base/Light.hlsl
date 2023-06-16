#ifndef _Light__
#define _Light__

struct LightData {
	float4 position;
	float4 direction;
	float3 color;
	uint lightType;
};

#endif