#ifndef _BackBufferPassTest__
#define _BackBufferPassTest__

#include "MainEntryPoint.hlsl"

struct a2v 
{
	float3 lsPos : POSITION;
	float3 lsNormal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct v2p
{
	float4 csPos : SV_POSITION;
	float2 uv : TEXCOORD;
};

v2p VSMain(a2v input)
{
	v2p output;
	output.csPos = float4(input.lsPos, 1.0f);
	output.csPos = mul(output.csPos, FrameDataCB.CurrentEditorCamera.ViewProjection);
	output.uv = input.uv;

	return output;
}

float4 PSMain(v2p input) : SV_TARGET
{
	FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> feedback = ResourceDescriptorHeap[0];
	Texture2D streamTexture = ResourceDescriptorHeap[1];

	feedback.WriteSamplerFeedback(streamTexture, SamplerPointClamp, input.uv);

	float3 value = streamTexture.Sample(SamplerAnisotropicClamp, input.uv, 0, 2).rgb;

	return float4(value, 1.0f);
}

#endif