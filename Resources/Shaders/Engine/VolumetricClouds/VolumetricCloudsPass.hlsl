#ifndef _VolumetricClouds__
#define _VolumetricClouds__

#include "VolumetricCloudsHelper.hlsl"

struct PassData{
	// 资源参数
	uint   weatherMapIndex;
	uint   shapeNoiseMapIndex;
	uint   detailNoiseMapIndex;
	uint   blueNoise2DMapIndex;
	uint2  blueNoise2DMapSize;
	uint   gBufferViewDepthMapIndex;
	uint   previousPassOutputMapIndex;
	uint   mipLevel;
	uint2  previousPassOutputDimension;
	float  pad1;
	// 控制参数
	float  cloudsBottomHeight;	// 云层底部高度
	float  cloudsLayHeight;		// 云层总层高
	float  scatterForward;
	float  scatterForwardIntensity;
	float  scatterBackward;
	float  scatterBackwardIntensity;
	float  scatterBase;
	float  scatterMultiply;
	uint   cloudRaymarchSteps;	// 视线方向步进次数
	uint   lightRaymarchSteps;	// 光照方向步进次数
	float  crispiness;
	float  curliness;
	float  coverage;
	float  absorption;
	float  densityFactor;
	float  cloudType;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

// 获取高度比率
float GetHeightFraction(float3 inPos) {
    float innerRadius = EarthRadius + PassDataCB.cloudsBottomHeight;
    float outerRadius = innerRadius + PassDataCB.cloudsLayHeight;
    return (length(inPos - EarthCenter) - innerRadius) / (outerRadius - innerRadius);
}

float2 GetUVProjection(float3 inPos) {
    float innerRadius = EarthRadius + PassDataCB.cloudsBottomHeight;
    return inPos.xz / innerRadius + 0.5f;
}

/*
采样云的密度
*/
float SampleCloudDensity(float3 inPos, bool useHighFreq, uint lod) {
	Texture2D<float4> weatherTex = ResourceDescriptorHeap[PassDataCB.weatherMapIndex];
	Texture3D<float4> cloudTex   = ResourceDescriptorHeap[PassDataCB.shapeNoiseMapIndex];
	Texture3D<float4> worleyTex  = ResourceDescriptorHeap[PassDataCB.detailNoiseMapIndex];

	float3 windDirection = FrameDataCB.WindParameters.xyz;
	float  windSpeed = FrameDataCB.WindParameters.w;

	float heightFraction = GetHeightFraction(inPos);
    float3 scroll = windDirection * (heightFraction * 750.0f + FrameDataCB.TotalTime * windSpeed);
    
    float2 UV = GetUVProjection(inPos);
    float2 dynamicUV = GetUVProjection(inPos + scroll);

    float3 wind = windDirection * windSpeed * FrameDataCB.TotalTime;
    float3 position = inPos + wind * 30;

    if (heightFraction < 0.0f || heightFraction > 1.0f) {
        return 0.0f;
    }

    // low frequency sample
    float4 lowFreqNoise = cloudTex.SampleLevel(SamplerLinearWrap, position * 0.0001f, lod);
    float lowFreqFBM = dot(lowFreqNoise.gba, float3(0.625, 0.25, 0.125));
    float cloudSample = Remap(lowFreqNoise.r, -(1.0f - lowFreqFBM), 1.0f, 0.0f, 1.0f);
	 
    float density = GetDensityForCloud(heightFraction, 1.0f);
    cloudSample *= (density / heightFraction);

    float3 weatherNoise = weatherTex.SampleLevel(SamplerLinearWrap, GetUVProjection(position), 0).rgb;
    if(weatherNoise.r < 0.0f) {
        return 0.0f;
    }
    float cloudWeatherCoverage = weatherNoise.r * PassDataCB.coverage * 0.30f;
    float cloudSampleWithCoverage = Remap(cloudSample, cloudWeatherCoverage, 1.0f, 0.0f, 1.0f);
    cloudSampleWithCoverage *= cloudWeatherCoverage;

    // high frequency sample
    if (useHighFreq) {
        //细节噪声受更强风的影响，添加稍微向上的偏移
        position += (windDirection + float3(0.0f, 0.1f, 0.0f)) * windSpeed * FrameDataCB.TotalTime * 0.1f;
        
        float3 highFreqNoise = worleyTex.SampleLevel(SamplerLinearWrap, position * 0.0001f, lod).rgb;
        float highFreqFBM = dot(highFreqNoise.rgb, float3(0.625, 0.25, 0.125));
        float highFreqNoiseModifier = lerp(highFreqFBM, 1.0f - highFreqFBM, clamp(heightFraction * 10.0f, 0.0f, 1.0f));
        // cloudSampleWithCoverage = cloudSampleWithCoverage - highFreqNoiseModifier * (1.0 - cloudSampleWithCoverage);
        cloudSampleWithCoverage = Remap(cloudSampleWithCoverage, highFreqNoiseModifier * 0.2, 1.0f, 0.0f, 1.0f);
    }

    return clamp(cloudSampleWithCoverage, 0.0f, 1.0f);
}

float RaymarchToLight(
	float3 origin, 
	float stepSize,
	float3 lightDir) {
    float3 startPos = origin;
    
    float deltaStep = stepSize * 6.0f;
    float3 rayStep = lightDir * deltaStep;
    const float coneStep = 1.0f / 6.0f;
    float coneRadius = 1.0f;
    float coneDensity = 0.0;
    
    float density = 0.0;
    const float densityThreshold = 0.3f;

    float3 pos;

    float finalTransmittance = 1.0f;

    for (int i = 0; i < 6; i++) {
        pos = startPos + coneRadius * NOISE_KERNEL_CONE_SAMPLING[i] * float(i);

        float heightFraction = GetHeightFraction(pos);
        if (heightFraction >= 0.0f) {

            float cloudSampleDensity = SampleCloudDensity(pos, density > densityThreshold, 0u);
            if (cloudSampleDensity > 0.0f) {
                float curTransmittance = Beer(cloudSampleDensity * deltaStep * PassDataCB.absorption, PassDataCB.absorption);
                finalTransmittance *= curTransmittance;
                density += cloudSampleDensity;
            }
        }
        startPos += rayStep;
        coneRadius += coneStep;
    }

    return finalTransmittance;
}

float4 RaymarchToCloud(
	uint2  globalCoord,
    float2 pixelUV,
	float3 startPos, 
	float3 endPos, 
	float3 skyColor) {

    const int steps = 64;
    
    float3 path = endPos - startPos;
    float len = length(path);
	
    float deltaStep = len / (float) steps;
    float3 dir = path / len;
    dir *= deltaStep;
    
    Texture2D<float4> blueNoise2DMap = ResourceDescriptorHeap[PassDataCB.blueNoise2DMapIndex];
	float  blueNoise = blueNoise2DMap.SampleLevel(SamplerLinearWrap, pixelUV, 0u).r;
    int a = int(globalCoord.x) % 4;
    int b = int(globalCoord.y) % 4;
    startPos += dir * blueNoise;

    float3 pos = startPos;

	// 获取太阳光信息
	Light  sunLight = LightDataSB[0];
	float3 wsLightDir = normalize(sunLight.position.xyz);
	float3 sunColor = sunLight.color;

    // 总亮度
    float3 totalLum = 0.0f;

    // 光照透射率
    float lightTransmittance = 1.0f;

    float LdotV = dot(normalize(wsLightDir), normalize(dir));

    float phase = HGScatterMax(dot(normalize(dir), wsLightDir), PassDataCB.scatterForward, PassDataCB.scatterForwardIntensity, PassDataCB.scatterBackward, PassDataCB.scatterBackwardIntensity);
    phase = PassDataCB.scatterBase + phase * PassDataCB.scatterMultiply;

    float4 finalColor = float4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0; i < steps; ++i) {
        float cloudSampleDensity = SampleCloudDensity(pos, true, 0u);
        float density = cloudSampleDensity * deltaStep * PassDataCB.densityFactor;
        // 当前采样点的接受的光照亮度
        float currentLum = 0.0f;

        if (density > 0.0f) {
            /*
            float transmittanceAlongLightDir = RaymarchToLight(pos, deltaStep * 0.1f, wsLightDir.rgb);

            float scattering = max(lerp(HenyeyGreenstein(LdotV, -0.08f), HenyeyGreenstein(LdotV, 0.08f), clamp(LdotV * 0.5f + 0.5f, 0.0f, 1.0f)), 1.0f);
            float3 ambientColor = float3(102.0f / 255.0f, 104.0f / 255.0f, 105.0f / 255.0f);

            float3 S = 0.6f * (lerp(lerp(ambientColor.rgb * 0.4f, skyColor, 0.0f), scattering * sunColor.rgb, transmittanceAlongLightDir)) * cloudSampleDensity;
            float deltaTransmittance = Beer(density, PassDataCB.absorption);
            float3 Sint = (S - S * deltaTransmittance) * (1.0f / cloudSampleDensity);
            totalLum += lightTransmittance * Sint;
            lightTransmittance *= deltaTransmittance;
            */

            // 当前采样点沿着光线方向的透射率
            float transmittanceAlongLightDir = RaymarchToLight(pos, deltaStep * 0.1f, wsLightDir.rgb);
            currentLum = 0.7f;
            currentLum = 0.3f + currentLum * (1.0f - 0.3f);
            //云层颜色
            float3 cloudColor = Interpolation3(float3(0.1f, 0.1f, 0.1f), float3(0.5f, 0.5f, 0.5f), float3(1.0f, 1.0f, 1.0f), saturate(currentLum), 0.7f) * sunColor;
                            
            totalLum += lightTransmittance * cloudColor * density * phase;
            lightTransmittance *= Beer(density, PassDataCB.absorption);
        }

        if (lightTransmittance <= 0.01f) {
            break;
        }
        
        pos += dir;
    }

    finalColor = float4(skyColor.rgb * lightTransmittance + totalLum, lightTransmittance);
    return finalColor;
}

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int groupIndex : SV_GroupIndex) {
	Texture2D<float4>   gBufferViewDepthMap   = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> previousPassOutputMap = ResourceDescriptorHeap[PassDataCB.previousPassOutputMapIndex];

	uint2 pixelIndex = dispatchThreadID.xy;
	float2 pixelUV = TexelIndexToUV(pixelIndex, PassDataCB.previousPassOutputDimension);

	float4 backColor   = previousPassOutputMap[pixelIndex].rgba;

	//compute ray direction
    float4 rayClipSpace = float4(UVToClipPosition(pixelUV), 1.0f);
    float4 rayView = mul(rayClipSpace, FrameDataCB.CurrentEditorCamera.InverseProjection);
    rayView = float4(rayView.xy, 1.0f, 0.0f);
    
    float3 worldDir = mul(rayView, FrameDataCB.CurrentEditorCamera.InverseView).xyz;
    worldDir = normalize(worldDir);
    
    float3 startPos, endPos;
    float  innerRadius = EarthRadius + PassDataCB.cloudsBottomHeight;
    float  outerRadius = innerRadius + PassDataCB.cloudsLayHeight;

	float3 cameraPos = FrameDataCB.CurrentEditorCamera.Position.xyz;
	float3 sphereCenter = float3(cameraPos.x, -EarthRadius, cameraPos.z);
	if (cameraPos.y < innerRadius - EarthRadius) {
        RaySphereIntersectionFromOriginPoint(cameraPos.xyz, worldDir, sphereCenter, innerRadius, startPos);
        RaySphereIntersectionFromOriginPoint(cameraPos.xyz, worldDir, sphereCenter, outerRadius, endPos);
    }
    else if (cameraPos.y > innerRadius - EarthRadius && cameraPos.y < outerRadius - EarthRadius) {
        startPos = cameraPos.xyz;
        RaySphereIntersectionFromOriginPoint(cameraPos.xyz, worldDir, sphereCenter, outerRadius, endPos);
    }
    else {
        RaySphereIntersectionFromOriginPoint(cameraPos.xyz, worldDir, sphereCenter, outerRadius, startPos);
        RaySphereIntersectionFromOriginPoint(cameraPos.xyz, worldDir, sphereCenter, innerRadius, endPos);
    }

    float  viewDepth   = gBufferViewDepthMap[pixelIndex].x;
	float3 wsObjectPos = ViewDepthToWorldPosition(viewDepth, pixelUV, FrameDataCB.CurrentEditorCamera);
	float  dstToObjectPos = length(wsObjectPos - cameraPos);
    float  dstToStartPos  = length(startPos - cameraPos);

    if(dstToObjectPos <= dstToStartPos || startPos.y <= 0.0f) {
        return;
    }

	float4 finalColor = RaymarchToCloud(pixelIndex, pixelUV, startPos, endPos, backColor.rgb);

    previousPassOutputMap[pixelIndex] = float4(finalColor.rgb, 1.0f);
}

#endif