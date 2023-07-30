#ifndef _OpaquePass__
#define _OpaquePass__

struct PassData {
	uint index1;
	uint index2;
	uint index3;
	uint index4;
};

#define PassDataType PassData

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
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

v2p VSMain(a2v input) {
	v2p output;

	// TODO ÐÞ¸Ä
	output.csPos = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return output;
}

p2o PSMain(v2p input) {
	p2o output;

	// TODO ÐÞ¸Ä
	output.shadingResult   = float4(0.0f, 0.0f, 0.0f, 0.0f);
	output.normalRoughness = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.screenVelocity    = float2(0.0f, 0.0f);

	return output;
}

#endif