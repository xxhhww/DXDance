#ifndef _GrassBladeRenderer__
#define _GrassBladeRenderer__

struct PassData {
	
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Math/MathCommon.hlsl"

struct v2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
    float3 wsPos     : POSITION2;
	float2 uv        : TEXCOORD0;
    float3 normal    : NORMAL0;
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

v2p VSMain(uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) {
}

p2o PSMain(v2p input) {
	
}

#endif