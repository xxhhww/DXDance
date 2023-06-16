#ifndef _Light__
#define _Light__

struct Light {
	float4 position;
	float4 direction;
	float3 color;
	uint lightType;
};

#endif