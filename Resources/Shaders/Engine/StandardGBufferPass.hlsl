#ifndef _StandardGBufferPass__
#define _StandardGBufferPass__

struct PassData {
	uint index1;
	uint index2;
	uint index3;
	uint index4;
};

#define PassDataType PassData

#include "Base/MainEntryPoint.hlsl"

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
	float4 albedo   : SV_Target0;
    float4 position : SV_Target1;	// world space position
    float4 normal   : SV_Target2;	// world space normal
    float4 mre      : SV_Target3;	// metallic + roughness + emission
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
	output.albedo   = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.position = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.normal   = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.mre      = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return output;
}

#endif