#ifndef _TerrainFeedbackRenderer__
#define _TerrainFeedbackRenderer__

struct PassData {
	float pad0;
	float pad1;
	float pad2;
	float pad3;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float4 color     : COLOR;
};

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
	float3 wsPos     : POSITION2;
	float3 vsPos     : POSITION3;
	float2 uv        : TEXCOORD2;
	uint   nodeLod   : NODELOD;
};

struct p2o {
	uint4  terrainFeedback : SV_TARGET0;
};

v2p VSMain(a2v input, uint instanceID : SV_InstanceID) {
	v2p output;
	output.currCsPos = float4(0.0f, 0.0f, 0.0f, 0.0f);

	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	output.terrainFeedback = uint4(0, 0, 0, 0);

	return output;
}

#endif