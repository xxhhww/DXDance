#ifndef _VolumetricCloudsHelper__
#define _VolumetricCloudsHelper__

#define CONE_STEP 0.1666666
static const float4 StratusGradient = float4(0.0f, 0.1f, 0.2f, 0.3f);
static const float4 StratocumulusGradient = float4(0.02f, 0.2f, 0.48f, 0.625f);
static const float4 CumulusGradient = float4(0.0f, 0.1625f, 0.88f, 0.98f);
static const float  EarthRadius = 6300000.0f;
static const float3 StaticEarthCenter = float3(0.0f, -EarthRadius, 0.0f);

float3 UVToClipPosition(float2 uv) {
    float2 ray;
    ray.x = 2.0f * uv.x - 1.0f;
    ray.y = 1.0f - uv.y * 2.0f;
    
    return float3(ray, 1.0f);
}

/*
����������ƿ��ཻ(�����뾶��ͬ��ͬ��Բ)
PS: earthCenter�ڴ���ǰ��Ҫ���������λ�Ʋ�����������
*/
bool RayIntersectCloudsLay(float3 rayOrigin, float3 rayDir, float3 earthCenter, float cloudsLayBottom, float cloudsLayTop, out float minRayT, out float maxRayT) {
	float innerRadius = EarthRadius + cloudsLayBottom;	// �ڻ��뾶
	float outerRadius = EarthRadius + cloudsLayTop;		// �⻷�뾶

	// ������Բ�����������һԪ���η���
	float3 oc = rayOrigin - earthCenter;
	float  b  = dot(rayDir, oc);
	float  c1 = dot(oc, oc) - innerRadius * innerRadius;
	float  sqrtOpInner = b * b - c1;
	if(sqrtOpInner < 0.0f) {
		return false;
	}
	float deInner = sqrt(sqrtOpInner);
    float solAInner = -b - deInner;
    float solBInner = -b + deInner;
	float maxSInner = max(solAInner, solBInner);
    
	// ������Բ�����������һԪ���η���
	float c2 = dot(oc, oc) - outerRadius * outerRadius;
	float sqrtOpOuter = b * b - c2;
	if(sqrtOpOuter < 0.0f) {
		return false;
	}
	float deOuter = sqrt(sqrtOpOuter);
    float solAOuter = -b - deOuter;
    float solBOuter = -b + deOuter;
    float maxSOuter = max(solAOuter, solBOuter);

	minRayT = min(maxSInner, maxSOuter);
	maxRayT = max(maxSInner, maxSOuter);
	return true;
}

#endif