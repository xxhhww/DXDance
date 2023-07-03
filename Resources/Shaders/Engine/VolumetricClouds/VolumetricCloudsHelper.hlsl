#ifndef _VolumetricCloudsHelper__
#define _VolumetricCloudsHelper__

#include "../Math/MathCommon.hlsl"

#define CONE_STEP 0.1666666
static const float4 StratusGradient = float4(0.0f, 0.1f, 0.2f, 0.3f);
static const float4 StratocumulusGradient = float4(0.02f, 0.2f, 0.48f, 0.625f);
static const float4 CumulusGradient = float4(0.0f, 0.1625f, 0.88f, 0.98f);
static const float  EarthRadius = 6300000.0f;
static const float3 StaticEarthCenter = float3(0.0f, -EarthRadius, 0.0f);

// ������ʱ���õ�����Ϣ
struct CloudSampleInfo {
    float3 position;                //����λ��
    float  baseShapeTiling;         //������״ƽ��
    float3 baseShapeRatio;          //������״����(����ȾģʽΪBakeʱ����)
    float  boundBoxScaleMax;        //��Χ������
    float3 boundBoxPosition;        //��Χ��λ��
    float  detailShapeTiling;       //ϸ����״ƽ��
    float  weatherTexTiling;        //��������ƽ��
    float2 weatherTexOffset;        //��������ƫ��
    float  baseShapeDetailEffect;   //������״ϸ��Ӱ��
    float  detailEffect;            //ϸ������Ӱ��
    float  densityMultiplier;       //�ܶȳ���(����)
    float  cloudDensityAdjust;      //���ܶȵ��������ڵ������������Ƶĸ�����, 0 ~ 0.5 ~ 1 => 0 ~ weatherTex.r ~ 1
    float  cloudAbsorbAdjust;       //��������Ӱ�죬���ڵ������������Ƶ�������, 0 ~ 0.5 ~ 1 => 0 ~ weatherTex.b ~ 1
    float3 windDirection;           //����
    float  windSpeed;               //����
    float2 cloudHeightMinMax;       //�Ƹ߶ȵ���С(x) ���(y)ֵ
    float3 stratusInfo;             //������Ϣ��������С�߶�(x)  �������߶�(y)  ���Ʊ�Ե��ǿ��(z)
    float3 cumulusInfo;             //������Ϣ�� ������С�߶�(x)  �������߶�(y)  ���Ʊ�Ե��ǿ��(z)
    float  cloudOffsetLower;        //�Ƶײ�ƫ��(����ȾģʽΪNo3DTexʱ����)
    float  cloudOffsetUpper;        //�ƶ���ƫ��(����ȾģʽΪNo3DTexʱ����)
    float  feather;                 //�Ʋ���(����ȾģʽΪNo3DTexʱ����)
    float3 sphereCenter;            //������������
    float  earthRadius;             //����뾶
};

//��������Ƶ���Ϣ
struct CloudResultInfo {
    float density;          //�ܶ�
    float absorptivity;     //������
};

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

/*
�����Ƶ��ܶ���Ϣ
*/
CloudResultInfo SampleCloudDensity(CloudSampleInfo csi) {
    CloudResultInfo cri;


    return cri;
}

//������ֵ����в�ֵ, value1 -> value2 -> value3�� offset�����м�ֵ(value2)��ƫ��
float Interpolation3(float value1, float value2, float value3, float x, float offset = 0.5f) {
    offset = clamp(offset, 0.0001f, 0.9999f);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0.0f, x - offset) / (1.0f - offset));
}

//������ֵ����в�ֵ, value1 -> value2 -> value3�� offset�����м�ֵ(value2)��ƫ��
float3 Interpolation3(float3 value1, float3 value2, float3 value3, float x, float offset = 0.5f) {
    offset = clamp(offset, 0.0001f, 0.9999f);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0.0f, x - offset) / (1.0f - offset));
}

float Beer(float density, float absorptivity) {
    return exp(-density * absorptivity);
}

// ����ЧӦ��ģ���Ƶ���ɢ��Ӱ��
float SugarPowder(float density, float absorptivity) {
    return 2.0 * exp(-density * absorptivity) * (1.0 - exp(-2.0 * density));
}

// Henyey-Greenstein��λ����
float HenyeyGreenstein(float angle, float g) {
    float g2 = g * g;
    return(1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * angle, 1.5));
}

// ����Henyey-Greensteinɢ�䣬ʹ��Max��ϡ�ͬʱ�����ǰ ���ɢ��
float HGScatterMax(float angle, float g_1, float intensity_1, float g_2, float intensity_2) {
    return max(intensity_1 * HenyeyGreenstein(angle, g_1), intensity_2 * HenyeyGreenstein(angle, g_2));
}

// ����Henyey-Greensteinɢ�䣬ʹ��Lerp��ϡ�ͬʱ�����ǰ ���ɢ��
float HGScatterLerp(float angle, float g_1, float g_2, float weight) {
    return lerp(HenyeyGreenstein(angle, g_1), HenyeyGreenstein(angle, g_2), weight);
}

// ��ȡ��������(û��ʹ�õ�)
float GetLightEnergy(float density, float absorptivity, float darknessThreshold) {
    float energy = SugarPowder(density, absorptivity);
    return darknessThreshold + (1.0 - darknessThreshold) * energy;
}

#endif