#ifndef _TerrainTextureBaker__
#define _TerrainTextureBaker__

#include "TerrainHeader.hlsl"

struct PassData{
	uint  terrainHeightMapIndex;
	uint  terrainNormalMapIndex;
	uint  terrainSplatMapIndex;
	float pad0;

	float4x4 mvpMatrix;

	float4 tileOffset;
	float4 blendOffset;

    uint  vertexCountPerAxis;
    float vertexSpaceInMeterSize;   // 地形两个顶点之间的间隔
    float terrainMeterSize;
    float terrainHeightScale;
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
	float4 currCsPos        : SV_POSITION;
	float2 uv               : TEXCOORD;
};

struct p2o {
	float4  terrainAlbedo : SV_TARGET0;
	float4  terrainNormal : SV_TARGET1;
};

v2p VSMain(a2v input) {
	
	float2 pos = saturate(mul(float4(input.lsPos, 1.0f), PassDataCB.mvpMatrix).xy);
	pos.y = 1 - pos.y;

	v2p output;
	output.currCsPos = float4(2.0f * pos - 1.0f, 0.5f, 1.0f);
	output.uv = input.uv;
	return output;
}

p2o PSMain(v2p input) {
	p2o output;
	return output;
}

#endif