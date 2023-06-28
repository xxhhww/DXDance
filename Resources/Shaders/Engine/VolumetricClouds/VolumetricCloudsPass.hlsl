#ifndef _VolumetricClouds__
#define _VolumetricClouds__

#include "VolumetricCloudsHelper.hlsl"

struct PassData{
	// 资源参数
	uint weatherMapIndex;
	uint cloudMapIndex;
	uint worleyMapIndex;
	uint gBufferViewDepthMapIndex;
	uint previousPassOutputMapIndex;
	uint mipLevel;
	uint2 previousPassOutputDimension;
	// 控制参数
	float cloudsBottomHeight;
	float cloudsTopHeight;
	float crispiness;
	float curliness;
	float coverage;
	float cloudType;
	float absorption;
	float densityFactor;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int groupIndex : SV_GroupIndex) {
	Texture2D<float4> gBufferViewDepthMap     = ResourceDescriptorHeap[PassDataCB.gBufferViewDepthMapIndex];
	RWTexture2D<float4> previousPassOutputMap = ResourceDescriptorHeap[PassDataCB.previousPassOutputMapIndex];

	uint2 pixelIndex = dispatchThreadID.xy;
	float2 pixelUV = TexelIndexToUV(pixelIndex, PassDataCB.previousPassOutputDimension);

	float4 csRay = float4(UVToClipPosition(pixelUV), 1.0f);
	float4 vsRay = float4(mul(csRay, FrameDataCB.CurrentEditorCamera.InverseProjection).xy, 1.0f, 0.0f);
	float3 wsRay = normalize(mul(vsRay, FrameDataCB.CurrentEditorCamera.InverseView).xyz);
	float  viewDepth   = gBufferViewDepthMap[pixelIndex].x;	//PS: 降采样时需要进行修改
	float3 cameraPos   = FrameDataCB.CurrentEditorCamera.Position.xyz;
	float3 earthCenter = float3(cameraPos.x, -EarthRadius, cameraPos.z);

	// 计算光线与云壳的相交结果
	float minRayT = 0.0f;
	float maxRayT = 0.0f;
	bool isIntersect = RayIntersectCloudsLay(cameraPos, wsRay, earthCenter, PassDataCB.cloudsBottomHeight, PassDataCB.cloudsTopHeight, minRayT, maxRayT);

	float3 startPoint = cameraPos + wsRay * minRayT;
	float3 endPoint   = cameraPos + wsRay * maxRayT;

	float3 vsStartPoint = mul(float4(startPoint, 0.0f), FrameDataCB.CurrentEditorCamera.View).xyz;

	// 光线不与云壳相交，或者光线在到达云壳之前被其他物体挡住
	if(!isIntersect || viewDepth <= vsStartPoint.z) {
		return;
	}

	previousPassOutputMap[pixelIndex] = float4(0.0f, 1.0f, 0.0f, 1.0f);
}

#endif