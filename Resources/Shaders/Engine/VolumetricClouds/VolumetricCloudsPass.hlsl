#ifndef _VolumetricClouds__
#define _VolumetricClouds__

#include "VolumetricCloudsHelper.hlsl"

struct PassData{
	// 资源参数
	uint   weatherMapIndex;
	uint   blueNoise2DMapIndex;
	uint2  blueNoise2DMapSize;
	float  pad1;
	uint   gBufferViewDepthMapIndex;
	uint   previousPassOutputMapIndex;
	uint   mipLevel;
	uint2  previousPassOutputDimension;
	uint2  pad2;
	// 控制参数
	float  cloudsBottomHeight;
	float  cloudsTopHeight;
	float  scatterForward;
	float  scatterForwardIntensity;
	float  scatterBackward;
	float  scatterBackwardIntensity;
	float  scatterBase;
	float  scatterMultiply;
	uint   cloudRaymarchSteps;	// 视线方向步进次数
	float  lightRaymarchSteps;	// 光照方向步进次数
	float  darknessThreshold;	// 暗部阈值
	float  colorCentralOffset;	// 中间颜色偏移 
	float4 colorBright;			// 亮面颜色
	float4 colorCentral;		// 中间颜色
	float4 colorDark;			// 暗面颜色
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

/*
填充云的采样信息
*/
CloudSampleInfo FillCloudSampleInfo() {
	CloudSampleInfo csi;

	// 填充...

	return csi;
}

/*
沿着光照方向进行步进
返回值返回沿光线方向累计的密度
*/
float RaymarchAlongLightDir(
	float3 wsOriginPos,
	float3 wsLightDir,
	float3 earthCenter,
	float  cloudsLayBottom,
	float  cloudsLayTop) {
	// 计算射线与云壳的相交结果
	float  minRayT = 0.0f;
	float  maxRayT = 0.0f;
	bool   isIntersect = RayIntersectCloudsLay(wsOriginPos, wsLightDir, earthCenter, cloudsLayBottom, cloudsLayTop, minRayT, maxRayT);

	// 沿着光线方向步进时，其起始点应该是在云壳中的，因此minRayT < 0.0f，maxRayT > 0.0f
	float3 wsStartPos = wsOriginPos;
	float3 wsEndPos   = wsOriginPos + wsLightDir * maxRayT;

	// 计算总的光线步进的方向矢量与其长度、单位方向矢量
	float3 raymarchPathVector     = wsEndPos - wsStartPos;
	float  raymarchPathLength     = length(raymarchPathVector);
	float3 raymarchPathUnitVector = raymarchPathVector / raymarchPathLength;

	// 计算单次光线步进的方向矢量与其长度
	float3 raymarchDeltaStepVector = raymarchPathVector / (float)PassDataCB.lightRaymarchSteps;
	float  raymarchDeltaStepLength = length(raymarchDeltaStepVector);

	// 填充云采样信息
	CloudSampleInfo csi = FillCloudSampleInfo();

	// 获得当前步进位置
	float3 wsCurrPos = wsStartPos;

	// 累计的总的密度
	float totalDensity = 0.0f;

	for (uint i = 0u; i < PassDataCB.lightRaymarchSteps; i++) {
        wsCurrPos += raymarchDeltaStepVector;
        csi.position = wsCurrPos;
        float density = SampleCloudDensity(csi).density * raymarchDeltaStepLength;
        totalDensity += density;
    }

	return totalDensity;
}

/*
沿着视线方向进行步进
*/
bool RaymarchAlongViewDir(
	float2 pixelUV,
	float3 wsCameraPos,
	float3 wsViewDir,
	float3 wsObjectPos,
	float3 earthCenter,
	float  cloudsLayBottom,
	float  cloudsLayTop,
	float4 backColor,
	out float4 finalColor) {
	// 计算射线与云壳的相交结果
	float  minRayT = 0.0f;
	float  maxRayT = 0.0f;
	bool   isIntersect = RayIntersectCloudsLay(wsCameraPos, wsViewDir, earthCenter, cloudsLayBottom, cloudsLayTop, minRayT, maxRayT);
	
	// 射线与云壳不相交
	if(!isIntersect) {
		return false;
	}

	// 相交点在视线的反方向
	if(minRayT < 0.0f && maxRayT < 0.0f) {
		return false;
	}

	// 计算Raymarch的开始点与结束点(如果minRayT < 0.0f则说明当前摄像机可能在云壳中)
	float3 wsStartPos = select(minRayT < 0.0f, wsCameraPos, wsCameraPos + wsViewDir * minRayT);
	float3 wsEndPos   = wsCameraPos + wsViewDir * maxRayT;

	// 计算摄像机位置到云壳/物体的距离
	float  dstToStartPos  = length(wsStartPos - wsCameraPos);
	float  dstToEndPos    = length(wsEndPos - wsCameraPos);
	float  dstToObjectPos = length(wsObjectPos - wsCameraPos);

	// 射线被物体挡住
	if(dstToObjectPos <= dstToStartPos) {
		return false;
	}

	// 计算总的光线步进的方向矢量与其长度、单位方向矢量
	float3 raymarchPathVector     = wsEndPos - wsStartPos;
	float  raymarchPathLength     = length(raymarchPathVector);
	float3 raymarchPathUnitVector = raymarchPathVector / raymarchPathLength;

	// 计算单次光线步进的方向矢量与其长度
	float3 raymarchDeltaStepVector = raymarchPathVector / (float)PassDataCB.cloudRaymarchSteps;
	float  raymarchDeltaStepLength = length(raymarchDeltaStepVector);

	// 获得步进当前位置(使用蓝噪声进行偏移)与摄像机到当前位置的距离
	Texture2D<float4> blueNoise2DMap = ResourceDescriptorHeap[PassDataCB.blueNoise2DMapIndex];
	float  blueNoise = blueNoise2DMap.SampleLevel(SamplerLinearWrap, pixelUV, 0u).r;
	float3 wsCurrPos =  wsStartPos + blueNoise * raymarchDeltaStepVector;
	float  dstToCurrPos = length(wsCurrPos - wsCameraPos);

	// 获取太阳光照信息
	Light  sunLight = LightDataSB[0];
	float3 wsLightDir = normalize(sunLight.position.xyz);
	float3 sunIlluminance = sunLight.color;

	// 计算向前/向后散射强度
	float phase = HGScatterMax(dot(wsViewDir, wsLightDir), PassDataCB.scatterForward, PassDataCB.scatterForwardIntensity, PassDataCB.scatterBackward, PassDataCB.scatterBackwardIntensity);
    phase = PassDataCB.scatterBase + phase * PassDataCB.scatterMultiply;

	// 累计总密度
    float  totalDensity = 0.0f;
    // 总亮度
    float3 totalLum = float3(0.0f, 0.0f, 0.0f);
    // 光照衰减
    float  lightAttenuation = 1.0f;

	// 填充云采样信息
	CloudSampleInfo csi = FillCloudSampleInfo();

	// 沿着视线方向进行步进
	for(uint i = 0u; i < PassDataCB.cloudRaymarchSteps; i++) {
		// 更新云采样信息中的位置信息并进行采样
		csi.position = wsCurrPos;
		CloudResultInfo cri = SampleCloudDensity(csi);

		// 云密度乘上单次步进的长度
		float density = cri.density * raymarchDeltaStepLength;
		
		// 当前位置受到的光照强度
		float currentLum = 0.0f;
		
		// 当前采样的位置存在云，则进行光照计算
		if(cri.density > 0.0f) {
			float totalDensityAlongLightDir = RaymarchAlongLightDir(wsCurrPos, wsLightDir, earthCenter, cloudsLayBottom, cloudsLayTop);
			currentLum = SugarPowder(totalDensityAlongLightDir, cri.absorptivity);
			currentLum = PassDataCB.darknessThreshold + currentLum * (1.0f - PassDataCB.darknessThreshold);
			
			// 计算云的颜色
			float3 cloudColor = Interpolation3(PassDataCB.colorDark.rgb, PassDataCB.colorCentral.rgb, PassDataCB.colorBright.rgb, saturate(currentLum), PassDataCB.colorCentralOffset) * sunIlluminance;
			
			// 对最终值进行迭代
			totalLum += lightAttenuation * cloudColor * density * phase;
			totalDensity += density;
			lightAttenuation *= Beer(density, cri.absorptivity);

			// 当光线衰减达到一定程度的时候跳出步进循环
			if (lightAttenuation < 0.01f) {
				break;
			}
		}

		// 更新步进当前位置与摄像机到当前位置的距离
		wsCurrPos += raymarchDeltaStepVector;
		dstToCurrPos = length(wsCurrPos - wsCameraPos);

		// 如果步进到被物体遮挡的位置，或者穿出云壳覆盖范围时，跳出循环
		if(dstToObjectPos <= dstToCurrPos || dstToEndPos <= dstToCurrPos) {
			break;
		}
	}

	// 返回步进后的最终结果
	finalColor = float4(backColor.rgb * lightAttenuation + totalLum, lightAttenuation);
	return true;
}

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int groupIndex : SV_GroupIndex) {
	Texture2D<float4>   gBufferViewDepthMap   = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> previousPassOutputMap = ResourceDescriptorHeap[PassDataCB.previousPassOutputMapIndex];

	uint2 pixelIndex = dispatchThreadID.xy;
	float2 pixelUV = TexelIndexToUV(pixelIndex, PassDataCB.previousPassOutputDimension);

	float4 backColor   = previousPassOutputMap[pixelIndex];

	float4 csViewDir   = float4(UVToClipPosition(pixelUV), 1.0f);
	float4 vsViewDir   = float4(mul(csViewDir, FrameDataCB.CurrentEditorCamera.InverseProjection).xy, 1.0f, 0.0f);
	float3 wsViewDir   = normalize(mul(vsViewDir, FrameDataCB.CurrentEditorCamera.InverseView).xyz);
	
	float  viewDepth   = gBufferViewDepthMap[pixelIndex].x;	// PS: 降采样时需要进行修改
	float3 wsObjectPos = ViewDepthToWorldPosition(viewDepth, pixelUV, FrameDataCB.CurrentEditorCamera);
	
	float3 wsCameraPos = FrameDataCB.CurrentEditorCamera.Position.xyz;
	float3 earthCenter = float3(wsCameraPos.x, -EarthRadius, wsCameraPos.z);


	float4 finalColor  = float4(0.0f, 0.0f, 0.0f, 0.0f);
	bool   isValid     = RaymarchAlongViewDir(
		pixelUV, wsCameraPos, wsViewDir, wsObjectPos, earthCenter,
		PassDataCB.cloudsBottomHeight, PassDataCB.cloudsTopHeight, backColor, finalColor
	);

	if(isValid) {
		previousPassOutputMap[pixelIndex] = float4(0.0f, 1.0f, 0.0f, 1.0f);
	}
}

#endif