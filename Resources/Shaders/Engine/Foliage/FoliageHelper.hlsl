#ifndef _FoliageHelper__
#define _FoliageHelper__

/*
���õ����Ϣ
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