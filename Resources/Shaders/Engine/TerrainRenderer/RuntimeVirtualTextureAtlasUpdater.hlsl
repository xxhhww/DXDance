#ifndef _RuntimeVirtualTextureAtlasUpdater__
#define _RuntimeVirtualTextureAtlasUpdater__

#include "TerrainHeader.hlsl"

struct PassData{
	uint drawRequestBufferIndex;
	uint terrainSplatMapIndex;
	uint terrainAlbedoTextureArrayIndex;
	uint terrainNormalTextureArrayIndex;

	uint terrainRoughnessTextureArrayIndex;
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
};

struct v2p {
	float4 currCsPos   : SV_POSITION;
	float2 uv          : TEXCOORD0;
	float4 blendTile   : Offset0;
	float4 tileOffset  : Offset1;
};

struct p2o {
	float4 albedo : SV_TARGET0;
	float4 normal : SV_TARGET1;
};



#endif