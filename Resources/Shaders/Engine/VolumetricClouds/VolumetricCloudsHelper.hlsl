#ifndef _VolumetricCloudsHelper__
#define _VolumetricCloudsHelper__

#include "../Math/MathCommon.hlsl"

static const float BAYER_FACTOR = 1.0f / 16.0f;
static const float BAYER_FILTER[16] = {
    0.0f * BAYER_FACTOR, 8.0f * BAYER_FACTOR, 2.0f * BAYER_FACTOR, 10.0f * BAYER_FACTOR,
	12.0f * BAYER_FACTOR, 4.0f * BAYER_FACTOR, 14.0f * BAYER_FACTOR, 6.0f * BAYER_FACTOR,
	3.0f * BAYER_FACTOR, 11.0f * BAYER_FACTOR, 1.0f * BAYER_FACTOR, 9.0f * BAYER_FACTOR,
	15.0f * BAYER_FACTOR, 7.0f * BAYER_FACTOR, 13.0f * BAYER_FACTOR, 5.0f * BAYER_FACTOR
};

// Cone sampling random offsets (for light)
static float3 NOISE_KERNEL_CONE_SAMPLING[6] = {
    float3(0.38051305, 0.92453449, -0.02111345),
	float3(-0.50625799, -0.03590792, -0.86163418),
	float3(-0.32509218, -0.94557439, 0.01428793),
	float3(0.09026238, -0.27376545, 0.95755165),
	float3(0.28128598, 0.42443639, -0.86065785),
	float3(-0.16852403, 0.14748697, 0.97460106)
};

static const float4 StratusGradient = float4(0.0f, 0.1f, 0.2f, 0.3f);
static const float4 StratocumulusGradient = float4(0.02f, 0.2f, 0.48f, 0.625f);
static const float4 CumulusGradient = float4(0.0f, 0.1625f, 0.88f, 0.98f);
static const float  EarthRadius = 600000.0f;
static const float3 EarthCenter = float3(0.0f, -EarthRadius, 0.0f);

float3 UVToClipPosition(float2 tex) {
    float2 ray;
    ray.x = 2.0f * tex.x - 1.0f;
    ray.y = 1.0f - tex.y * 2.0f;
    
    return float3(ray, 1.0f);
}

/*
处理光线与圆相交
*/
bool RayIntersectSphere(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius, out float minRayT, out float maxRayT) {
    float3 oc = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, oc);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;

    float discr = b * b - 4.0 * a * c;
    if (discr < 0.0f) {
        return false;
    }

    float x1 = (-b + sqrt(discr)) / 2.0f;
    float x2 = (-b - sqrt(discr)) / 2.0f;

    minRayT = min(x1, x2);
    maxRayT = max(x1, x2);

    return true;
}

bool RaySphereIntersectionFromOriginPoint(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius, out float3 posHit) {
    float t;
    float radius2 = sphereRadius * sphereRadius;

    float3 L = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, L);
    float c = dot(L, L) - radius2;

    float discr = b * b - 4.0 * a * c;
    if (discr < 0.0)
        return false;
    t = max(0.0, (-b + sqrt(discr)) / 2);
    if (t == 0.0)
    {
        return false;
    }
    posHit = rayOrigin + rayDir * t;

    return true;
}

/*
处理光线与云壳相交(两个半径不同的同心圆)
PS: earthCenter在传入前需要由摄像机的位移参数进行修正
*/
bool RayIntersectCloudsLay(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float cloudsBottomHeight, float cloudsLayHeight, out float minRayT, out float maxRayT) {
	float innerRadius = EarthRadius + cloudsBottomHeight;   // 内环半径
	float outerRadius = innerRadius + cloudsLayHeight;		// 外环半径

    float innerMinRayT;
    float innerMaxRayT;

    float outerMinRayT;
    float outerMaxRayT;

    bool isValid;

    isValid = RayIntersectSphere(rayOrigin, rayDir, sphereCenter, innerRadius, innerMinRayT, innerMaxRayT);
    if(isValid == false) {
        return false;
    }

    isValid = RayIntersectSphere(rayOrigin, rayDir, sphereCenter, outerRadius, outerMinRayT, outerMaxRayT);
    if(isValid == false) {
        return false;
    }

    minRayT = innerMaxRayT;
    maxRayT = outerMaxRayT;

	return true;
}

float GetDensityForCloud(float heightFraction, float cloudType) {
    float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
    float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
    float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

    float4 baseGradient = stratusFactor * StratusGradient + stratoCumulusFactor * StratocumulusGradient + cumulusFactor * CumulusGradient;
    return smoothstep(baseGradient.x, baseGradient.y, heightFraction) - smoothstep(baseGradient.z, baseGradient.w, heightFraction);
}

//在三个值间进行插值, value1 -> value2 -> value3， offset用于中间值(value2)的偏移
float Interpolation3(float value1, float value2, float value3, float x, float offset = 0.5) {
    offset = clamp(offset, 0.0001, 0.9999);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0, x - offset) / (1.0 - offset));
}

//在三个值间进行插值, value1 -> value2 -> value3， offset用于中间值(value2)的偏移
float3 Interpolation3(float3 value1, float3 value2, float3 value3, float x, float offset = 0.5) {
    offset = clamp(offset, 0.0001, 0.9999);
    return lerp(lerp(value1, value2, min(x, offset) / offset), value3, max(0, x - offset) / (1.0 - offset));
}

// 重映射
float Remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax) {
    return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float Beer(float density, float absorptivity) {
    return exp(-density * 1);
}

float BeerPowder(float density, float absorptivity) {
    return 2.0 * exp(-density * 1) * (1.0 - exp(-2.0f * density));
}

float HenyeyGreenstein(float angle, float g) {
    float g2 = g * g;
    return(1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * angle, 1.5));
}

//两层Henyey-Greenstein散射，使用Max混合。同时兼顾向前 向后散射
float HGScatterMax(float angle, float g_1, float intensity_1, float g_2, float intensity_2) {
    return max(intensity_1 * HenyeyGreenstein(angle, g_1), intensity_2 * HenyeyGreenstein(angle, g_2));
}

#endif