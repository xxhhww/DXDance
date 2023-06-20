#ifndef _OutputBackBuffer__
#define _OutputBackBuffer__

#include "../Base/MainEntryPoint.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p {
	float4 csPos : SV_POSITION;
	float2 uv : TEXCOORD;
};

v2p VSMain(a2v input) {
	v2p output;
	output.csPos = float4(input.lsPos, 1.0f);
	output.uv = input.uv;

	return output;
}

float4 PSMain(v2p input) : SV_TARGET {
	Texture2D finalOutputTexture = ResourceDescriptorHeap[18];
	float3 value = finalOutputTexture.Sample(SamplerPointClamp, input.uv).rgb;

	return float4(value, 1.0f);
}

#endif