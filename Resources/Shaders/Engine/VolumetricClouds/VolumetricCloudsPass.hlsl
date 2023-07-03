#ifndef _VolumetricClouds__
#define _VolumetricClouds__

#include "VolumetricCloudsHelper.hlsl"

struct PassData{
	// ��Դ����
	uint   weatherMapIndex;
	uint   blueNoise2DMapIndex;
	uint2  blueNoise2DMapSize;
	float  pad1;
	uint   gBufferViewDepthMapIndex;
	uint   previousPassOutputMapIndex;
	uint   mipLevel;
	uint2  previousPassOutputDimension;
	uint2  pad2;
	// ���Ʋ���
	float  cloudsBottomHeight;
	float  cloudsTopHeight;
	float  scatterForward;
	float  scatterForwardIntensity;
	float  scatterBackward;
	float  scatterBackwardIntensity;
	float  scatterBase;
	float  scatterMultiply;
	uint   cloudRaymarchSteps;	// ���߷��򲽽�����
	float  lightRaymarchSteps;	// ���շ��򲽽�����
	float  darknessThreshold;	// ������ֵ
	float  colorCentralOffset;	// �м���ɫƫ�� 
	float4 colorBright;			// ������ɫ
	float4 colorCentral;		// �м���ɫ
	float4 colorDark;			// ������ɫ
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

/*
����ƵĲ�����Ϣ
*/
CloudSampleInfo FillCloudSampleInfo() {
	CloudSampleInfo csi;

	// ���...

	return csi;
}

/*
���Ź��շ�����в���
����ֵ�����ع��߷����ۼƵ��ܶ�
*/
float RaymarchAlongLightDir(
	float3 wsOriginPos,
	float3 wsLightDir,
	float3 earthCenter,
	float  cloudsLayBottom,
	float  cloudsLayTop) {
	// �����������ƿǵ��ཻ���
	float  minRayT = 0.0f;
	float  maxRayT = 0.0f;
	bool   isIntersect = RayIntersectCloudsLay(wsOriginPos, wsLightDir, earthCenter, cloudsLayBottom, cloudsLayTop, minRayT, maxRayT);

	// ���Ź��߷��򲽽�ʱ������ʼ��Ӧ�������ƿ��еģ����minRayT < 0.0f��maxRayT > 0.0f
	float3 wsStartPos = wsOriginPos;
	float3 wsEndPos   = wsOriginPos + wsLightDir * maxRayT;

	// �����ܵĹ��߲����ķ���ʸ�����䳤�ȡ���λ����ʸ��
	float3 raymarchPathVector     = wsEndPos - wsStartPos;
	float  raymarchPathLength     = length(raymarchPathVector);
	float3 raymarchPathUnitVector = raymarchPathVector / raymarchPathLength;

	// ���㵥�ι��߲����ķ���ʸ�����䳤��
	float3 raymarchDeltaStepVector = raymarchPathVector / (float)PassDataCB.lightRaymarchSteps;
	float  raymarchDeltaStepLength = length(raymarchDeltaStepVector);

	// ����Ʋ�����Ϣ
	CloudSampleInfo csi = FillCloudSampleInfo();

	// ��õ�ǰ����λ��
	float3 wsCurrPos = wsStartPos;

	// �ۼƵ��ܵ��ܶ�
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
�������߷�����в���
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
	// �����������ƿǵ��ཻ���
	float  minRayT = 0.0f;
	float  maxRayT = 0.0f;
	bool   isIntersect = RayIntersectCloudsLay(wsCameraPos, wsViewDir, earthCenter, cloudsLayBottom, cloudsLayTop, minRayT, maxRayT);
	
	// �������ƿǲ��ཻ
	if(!isIntersect) {
		return false;
	}

	// �ཻ�������ߵķ�����
	if(minRayT < 0.0f && maxRayT < 0.0f) {
		return false;
	}

	// ����Raymarch�Ŀ�ʼ���������(���minRayT < 0.0f��˵����ǰ������������ƿ���)
	float3 wsStartPos = select(minRayT < 0.0f, wsCameraPos, wsCameraPos + wsViewDir * minRayT);
	float3 wsEndPos   = wsCameraPos + wsViewDir * maxRayT;

	// ���������λ�õ��ƿ�/����ľ���
	float  dstToStartPos  = length(wsStartPos - wsCameraPos);
	float  dstToEndPos    = length(wsEndPos - wsCameraPos);
	float  dstToObjectPos = length(wsObjectPos - wsCameraPos);

	// ���߱����嵲ס
	if(dstToObjectPos <= dstToStartPos) {
		return false;
	}

	// �����ܵĹ��߲����ķ���ʸ�����䳤�ȡ���λ����ʸ��
	float3 raymarchPathVector     = wsEndPos - wsStartPos;
	float  raymarchPathLength     = length(raymarchPathVector);
	float3 raymarchPathUnitVector = raymarchPathVector / raymarchPathLength;

	// ���㵥�ι��߲����ķ���ʸ�����䳤��
	float3 raymarchDeltaStepVector = raymarchPathVector / (float)PassDataCB.cloudRaymarchSteps;
	float  raymarchDeltaStepLength = length(raymarchDeltaStepVector);

	// ��ò�����ǰλ��(ʹ������������ƫ��)�����������ǰλ�õľ���
	Texture2D<float4> blueNoise2DMap = ResourceDescriptorHeap[PassDataCB.blueNoise2DMapIndex];
	float  blueNoise = blueNoise2DMap.SampleLevel(SamplerLinearWrap, pixelUV, 0u).r;
	float3 wsCurrPos =  wsStartPos + blueNoise * raymarchDeltaStepVector;
	float  dstToCurrPos = length(wsCurrPos - wsCameraPos);

	// ��ȡ̫��������Ϣ
	Light  sunLight = LightDataSB[0];
	float3 wsLightDir = normalize(sunLight.position.xyz);
	float3 sunIlluminance = sunLight.color;

	// ������ǰ/���ɢ��ǿ��
	float phase = HGScatterMax(dot(wsViewDir, wsLightDir), PassDataCB.scatterForward, PassDataCB.scatterForwardIntensity, PassDataCB.scatterBackward, PassDataCB.scatterBackwardIntensity);
    phase = PassDataCB.scatterBase + phase * PassDataCB.scatterMultiply;

	// �ۼ����ܶ�
    float  totalDensity = 0.0f;
    // ������
    float3 totalLum = float3(0.0f, 0.0f, 0.0f);
    // ����˥��
    float  lightAttenuation = 1.0f;

	// ����Ʋ�����Ϣ
	CloudSampleInfo csi = FillCloudSampleInfo();

	// �������߷�����в���
	for(uint i = 0u; i < PassDataCB.cloudRaymarchSteps; i++) {
		// �����Ʋ�����Ϣ�е�λ����Ϣ�����в���
		csi.position = wsCurrPos;
		CloudResultInfo cri = SampleCloudDensity(csi);

		// ���ܶȳ��ϵ��β����ĳ���
		float density = cri.density * raymarchDeltaStepLength;
		
		// ��ǰλ���ܵ��Ĺ���ǿ��
		float currentLum = 0.0f;
		
		// ��ǰ������λ�ô����ƣ�����й��ռ���
		if(cri.density > 0.0f) {
			float totalDensityAlongLightDir = RaymarchAlongLightDir(wsCurrPos, wsLightDir, earthCenter, cloudsLayBottom, cloudsLayTop);
			currentLum = SugarPowder(totalDensityAlongLightDir, cri.absorptivity);
			currentLum = PassDataCB.darknessThreshold + currentLum * (1.0f - PassDataCB.darknessThreshold);
			
			// �����Ƶ���ɫ
			float3 cloudColor = Interpolation3(PassDataCB.colorDark.rgb, PassDataCB.colorCentral.rgb, PassDataCB.colorBright.rgb, saturate(currentLum), PassDataCB.colorCentralOffset) * sunIlluminance;
			
			// ������ֵ���е���
			totalLum += lightAttenuation * cloudColor * density * phase;
			totalDensity += density;
			lightAttenuation *= Beer(density, cri.absorptivity);

			// ������˥���ﵽһ���̶ȵ�ʱ����������ѭ��
			if (lightAttenuation < 0.01f) {
				break;
			}
		}

		// ���²�����ǰλ�������������ǰλ�õľ���
		wsCurrPos += raymarchDeltaStepVector;
		dstToCurrPos = length(wsCurrPos - wsCameraPos);

		// ����������������ڵ���λ�ã����ߴ����ƿǸ��Ƿ�Χʱ������ѭ��
		if(dstToObjectPos <= dstToCurrPos || dstToEndPos <= dstToCurrPos) {
			break;
		}
	}

	// ���ز���������ս��
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
	
	float  viewDepth   = gBufferViewDepthMap[pixelIndex].x;	// PS: ������ʱ��Ҫ�����޸�
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