#ifndef _Light__
#define _Light__

struct Light {
	float4 position;	// sun direction + sunDiskArea
	float3 color;		// sun illuminance
	uint type;
};

#endif