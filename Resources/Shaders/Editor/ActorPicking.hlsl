#ifndef _ActorPicking__
#define _ActorPicking__

struct PassData {
	float4x4 modelMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float3   viewPos;
	int      actorID;
};
#define PassDataType PassData

#include "../Engine/Base/MainEntryPoint.hlsl"

struct a2v{
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p{
	float4 csPos : SV_POSITION;
	float3 color : COLOR;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID)
{
	v2p output;

	float4 wsPos = mul(float4(input.lsPos, 1.0f), PassDataCB.modelMatrix);
	float4 vsPos = mul(wsPos, PassDataCB.viewMatrix);
	output.csPos = mul(vsPos, PassDataCB.projMatrix);
	output.color = float3(1.0f, 1.0f, PassDataCB.actorID / 255.0f);

	return output;
}

float4 PSMain(v2p input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}

#endif