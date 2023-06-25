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
	float4 albedoMetalness  : SV_TARGET0;
    float4 positionEmission : SV_TARGET1;	// world space position
    float4 normalRoughness  : SV_TARGET2;	// world space normal
	float4 motionVector     : SV_TARGET3; 
	float  viewDepth        : SV_TARGET4;
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
	output.albedoMetalness  = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.positionEmission = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.normalRoughness  = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.motionVector     = float4(0.0f, 0.0f, 0.0f, 0.0f);
	output.viewDepth        = 5000.0f;

	return output;
}

#endif