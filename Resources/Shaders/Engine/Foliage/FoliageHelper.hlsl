#ifndef _FoliageHelper__
#define _FoliageHelper__

/*
安置点的信息
*/
struct Placement {
	float4   position;
	float4   normal;
	float4x4 modelTrans;

	uint	 albedoMapIndex;
	float    pad1;
	float    pad2;
	float    pad3;
};

#endif