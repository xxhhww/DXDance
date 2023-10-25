#ifndef _ProceduralGrassHelper__
#define _ProceduralGrassHelper__

struct GrassBlade {
public:
	float3   position;
	float2   facing;

	float    height;
	float    width;
	float    tilt;		// ������Ҷ����б״̬
	float    bend;		// ���Ʋ�Ҷ������(��ʵ���ǿ��Ʊ�������������)
	float    sideCurve;	// ���Ʋ�Ҷ�ıߵ�����
};

struct ClumpParameter {
	float pullToCentre;
	float pointInSameDirection;
	float baseHeight;
	float heightRandom;
	float baseWidth;
	float widthRandom;
	float baseTilt;
	float tiltRandom;
	float baseBend;
	float bendRandom;
};

float2 hashwithoutsine22(float2 p) {
	float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return frac((p3.xx+p3.yz)*p3.zy);
}

float rand(float3 co) {
	return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
}

float remap01_neg11(float t){ 
    return (t * 2.0f) - 1.0f;
}

#endif