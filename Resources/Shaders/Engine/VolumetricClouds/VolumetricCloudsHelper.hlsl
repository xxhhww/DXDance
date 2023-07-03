#ifndef _VolumetricCloudsHelper__
#define _VolumetricCloudsHelper__

#include "../Math/MathCommon.hlsl"

#define CONE_STEP 0.1666666
static const float4 StratusGradient = float4(0.0f, 0.1f, 0.2f, 0.3f);
static const float4 StratocumulusGradient = float4(0.02f, 0.2f, 0.48f, 0.625f);
static const float4 CumulusGradient = float4(0.0f, 0.1625f, 0.88f, 0.98f);
static const float  EarthRadius = 6300000.0f;
static const float3 StaticEarthCenter = float3(0.0f, -EarthRadius, 0.0f);

// 采样云时所用到的信息
struct CloudSampleInfo {
    float3 position;                //采样位置
    float  baseShapeTiling;         //基础形状平铺
    float3 baseShapeRatio;          //基础形状比例(当渲染模式为Bake时启用)
    float  boundBoxScaleMax;        //包围盒缩放
    float3 boundBoxPosition;        //包围盒位置
    float  detailShapeTiling;       //细节形状平铺
    float  weatherTexTiling;        //天气纹理平铺
    float2 weatherTexOffset;        //天气纹理偏移
    float  baseShapeDetailEffect;   //基础形状细节影响
    float  detailEffect;            //细节噪声影响
    float  densityMultiplier;       //密度乘数(缩放)
    float  cloudDensityAdjust;      //云密度调整，用于调整天气纹理云的覆盖率, 0 ~ 0.5 ~ 1 => 0 ~ weatherTex.r ~ 1
    float  cloudAbsorbAdjust;       //云吸收率影响，用于调整天气纹理云的吸收率, 0 ~ 0.5 ~ 1 => 0 ~ weatherTex.b ~ 1
    float3 windDirection;           //风向
    float  windSpeed;               //风速
    float2 cloudHeightMinMax;       //云高度的最小(x) 最大(y)值
    float3 stratusInfo;             //层云信息，层云最小高度(x)  层云最大高度(y)  层云边缘羽化强度(z)
    float3 cumulusInfo;             //积云信息， 积云最小高度(x)  积云最大高度(y)  积云边缘羽化强度(z)
    float  cloudOffsetLower;        //云底部偏移(当渲染模式为No3DTex时启用)
    float  cloudOffsetUpper;        //云顶部偏移(当渲染模式为No3DTex时启用)
    float  feather;                 //云层羽化(当渲染模式为No3DTex时启用)
    float3 sphereCenter;            //地球中心坐标
    float  earthRadius;             //地球半径
};

//采样完后云的信息
struct CloudResultInfo {
    float density;          //密度
    float absorptivity;     //吸收率
};

float3 UVToClipPosition(float2 uv) {
    float2 ray;
    ray.x = 2.0f * uv.x - 1.0f;
    ray.y = 1.0f - uv.y * 2.0f;
    
    return float3(ray, 1.0f);
}

/*
处理光线与云壳相交(两个半径不同的同心圆)
PS: earthCenter在传入前需要由摄像机的位移参数进行修正
*/
bool RayIntersectCloudsLay(float3 rayOrigin, float3 rayDir, float3 earthCenter, float cloudsLayBottom, float cloudsLayTop, out float minRayT, out float maxRayT) {
	float innerRadius = EarthRadius + cloudsLayBottom;	// 内环半径
	float outerRadius = EarthRadius + cloudsLayTop;		// 外环半径

	// 解算内圆与光线联立的一元二次方程
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
    
	// 解算外圆与光线联立的一元二次方程
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
采样云的密度信息
*/
CloudResultInfo SampleCloudDensity(CloudSampleInfo csi) {
    CloudResultInfo cri;


    return cri;
}

//在三个值间进行插值, value1 -> value2 -> value3， offset用于中间值(value2)的偏移
float Interpolation3(float value1, float value2, float value3, float x, float offset = 0.5f) {
    offset = clamp(offset, 0.0001f, 0.9999f);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0.0f, x - offset) / (1.0f - offset));
}

//在三个值间进行插值, value1 -> value2 -> value3， offset用于中间值(value2)的偏移
float3 Interpolation3(float3 value1, float3 value2, float3 value3, float x, float offset = 0.5f) {
    offset = clamp(offset, 0.0001f, 0.9999f);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0.0f, x - offset) / (1.0f - offset));
}

float Beer(float density, float absorptivity) {
    return exp(-density * absorptivity);
}

// 粉糖效应，模拟云的内散射影响
float SugarPowder(float density, float absorptivity) {
    return 2.0 * exp(-density * absorptivity) * (1.0 - exp(-2.0 * density));
}

// Henyey-Greenstein相位函数
float HenyeyGreenstein(float angle, float g) {
    float g2 = g * g;
    return(1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * angle, 1.5));
}

// 两层Henyey-Greenstein散射，使用Max混合。同时兼顾向前 向后散射
float HGScatterMax(float angle, float g_1, float intensity_1, float g_2, float intensity_2) {
    return max(intensity_1 * HenyeyGreenstein(angle, g_1), intensity_2 * HenyeyGreenstein(angle, g_2));
}

// 两层Henyey-Greenstein散射，使用Lerp混合。同时兼顾向前 向后散射
float HGScatterLerp(float angle, float g_1, float g_2, float weight) {
    return lerp(HenyeyGreenstein(angle, g_1), HenyeyGreenstein(angle, g_2), weight);
}

// 获取光照亮度(没有使用到)
float GetLightEnergy(float density, float absorptivity, float darknessThreshold) {
    float energy = SugarPowder(density, absorptivity);
    return darknessThreshold + (1.0 - darknessThreshold) * energy;
}

#endif