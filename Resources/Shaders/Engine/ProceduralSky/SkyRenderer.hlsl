#ifndef _SkyRenderer__
#define _SkyRenderer__

#include "SkyHelper.hlsl"
#include "../Base/Light.hlsl"

struct PassData{

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
	float4 currCsPos : SV_Position;
	float3 uv		 : TEXCOORD0;
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float2 screenVelocity  : SV_TARGET1;	
};

v2p VSMain(uint vertexID : SV_VERTEXID) {
	v2p output;

	uint b = (1u << vertexID);
	float3 pos = float3(
		(0x287a & b) != 0, 
		(0x02af & b) != 0, 
		(0x31e3 & b) != 0
	) * 2.f - 1.f;

	output.uv = pos;
	output.currCsPos = float4(pos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);
	output.currCsPos.z = output.currCsPos.w - 1e-6f;

	return output;
}

p2o PSMain(v2p input) {
	Light sunLight = LightDataSB[0];

	float3 V = normalize(input.uv);
	float3 L = sunLight.position.xyz;

	float3 color = proceduralSky(V, L);

	color *= sunLight.intensity;

	p2o output;
	output.shadingResult = float4(max(color, 0.f), 1.f);
	output.screenVelocity = float2(0.0f, 0.0f);

	return output;
}

#endif