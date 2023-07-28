#ifndef _FoliageHelper__
#define _FoliageHelper__

/*
���õ����Ϣ
*/
struct Placement {
	float4   position;
	float4   normal;

	uint	 albedoMapIndex;
	uint     normalMapIndex;
	float    pad1;
	float    pad2;
};

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

#endif