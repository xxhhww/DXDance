#ifndef _GrassHelper__
#define _GrassHelper__

struct GrassVertex {
	float3 position;
	float2 uv;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float4 color;

	float4 userData;
};

struct GrassBlade {
	float3 position;
	float2 facing;

	float  windStrength;

	float  hash;

	uint   type;

	float  height;
	float  width;
	float  tilt;		// 描述草叶的倾斜状态
	float  bend;

	float  sideCurve;
};

struct BakedGrassBlade {
	float3 position;
	float2 facing;

	float  hash;
	uint   type;

	float  height;
	float  width;
	float  tilt;		// 描述草叶的倾斜状态
	float  bend;		// 控制草叶的弯曲(其实就是控制贝塞尔样条曲线)
	float  sideCurve;	// 控制草叶的边的弯曲

	// float  clumpCenterDistanceFade	// 到草群中心的归一化距离
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

float3 cubicBezier(float3 p0, float3 p1, float3 p2, float3 p3, float t) {
    float3 a = lerp(p0, p1, t);
    float3 b = lerp(p2, p3, t);
    float3 c = lerp(p1, p2, t);
    float3 d = lerp(a, c, t);
    float3 e = lerp(c, b, t);
    return lerp(d, e, t); 
}

/*
计算贝塞尔切线
*/
float3 bezierTangent(float3 p0, float3 p1, float3 p2, float3 p3, float t) {
    float omt  = 1.0f - t;
    float omt2 = omt * omt;
    float t2   = t * t;

    float3 tangent = 
        p0* (-omt2) +
        p1 * (3 * omt2 - 2 *omt) +
        p2 * (-3 * t2 + 2 * t) +
        p3 * (t2);
                     
    return normalize(tangent);
}

float3x3 AngleAxis3x3(float angle, float3 axis) {
	float c, s;
	sincos(angle, s, c);

	float t = 1 - c;
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;

	return float3x3(
		t * x * x + c, t * x * y - s * z, t * x * z + s * y,
		t * x * y + s * z, t * y * y + c, t * y * z - s * x,
		t * x * z - s * y, t * y * z + s * x, t * z * z + c
		);
}

#endif