#ifndef _UpdateRvtTiledMap__
#define _UpdateRvtTiledMap__

#include "RvtHelper.hlsl"

struct PassData{
	uint32_t drawRequestBufferIndex;
	uint32_t terrainHeightMapIndex;
	uint32_t terrainNormalMapIndex;
	uint32_t terrainSplatMapIndex;

	float2 terrainMeterSize;
	float  terrainHeightScale;
	float  pad;

	uint32_t rChannelAlbedoMapIndex;
	uint32_t rChannelNormalMapIndex;
	uint32_t rChannelRoughnessMapIndex;
	uint32_t rChannelHeightMapIndex;

	uint32_t gChannelAlbedoMapIndex;
	uint32_t gChannelNormalMapIndex;
	uint32_t gChannelRoughnessMapIndex;
	uint32_t gChannelHeightMapIndex;

	uint32_t bChannelAlbedoMapIndex;
	uint32_t bChannelNormalMapIndex;
	uint32_t bChannelRoughnessMapIndex;
	uint32_t bChannelHeightMapIndex;

	uint32_t aChannelAlbedoMapIndex;
	uint32_t aChannelNormalMapIndex;
	uint32_t aChannelRoughnessMapIndex;
	uint32_t aChannelHeightMapIndex;
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
	float4 currCsPos : SV_POSITION;
	float3 wsPos     : POSITION1;
	float2 heightUV  : TEXCOORD0;
};

struct p2o {
	float4 albedo : SV_TARGET0;
	float4 normal : SV_TARGET1;
};

float3 SampleTerrainNormalMap(float2 uv) {
	Texture2D<float4> normalMap = ResourceDescriptorHeap[PassDataCB.terrainNormalMapIndex];

    float3 normal;
    normal.xz = normalMap.SampleLevel(SamplerAnisotropicWrap, uv, 0u).xy * 2.0f - 1.0f;
    normal.y = sqrt(max(0u, 1u - dot(normal.xz, normal.xz)));

    return normalize(normal);
}

float4 SampleTerrainSplatMap(float2 uv) {
	// uv.x = 1.0f - uv.x;
	uv.y = 1.0f - uv.y;

	Texture2D<float4> splatMap = ResourceDescriptorHeap[PassDataCB.terrainSplatMapIndex];

	float4 splat = splatMap.SampleLevel(SamplerLinearWrap, uv, 0u).rgba;

	return splat;
}

float3 SampleColorMapWithTriMapping(Texture2D colorMap, float3 position, float3 normal, float3 textureScale, float sharpness = 15.0f) {
	float2 uvX = position.zy * textureScale.x;
	float2 uvY = position.xz * textureScale.y;
	float2 uvZ = position.xy * textureScale.z;

	float3 weights = pow(abs(normal), sharpness);
	weights /= dot(weights, 1.0f);

	float3 xDiff = colorMap.SampleLevel(SamplerLinearWrap, uvX, 0).rgb;
    float3 yDiff = colorMap.SampleLevel(SamplerLinearWrap, uvY, 0).rgb;
    float3 zDiff = colorMap.SampleLevel(SamplerLinearWrap, uvZ, 0).rgb;

	float3 color = xDiff * weights.x + yDiff * weights.y + zDiff * weights.z;

	return color;
}

float SampleHeightMapWithTriMapping(Texture2D heightMap, float3 position, float3 normal, float3 textureScale, float sharpness = 15.0f) {
	float2 uvX = position.zy * textureScale.x;
	float2 uvY = position.xz * textureScale.y;
	float2 uvZ = position.xy * textureScale.z;

	float3 weights = pow(abs(normal), sharpness);
	weights /= dot(weights, 1.0f);

	float xDiff = heightMap.SampleLevel(SamplerLinearWrap, uvX, 0).r;
    float yDiff = heightMap.SampleLevel(SamplerLinearWrap, uvY, 0).r;
    float zDiff = heightMap.SampleLevel(SamplerLinearWrap, uvZ, 0).r;

	float height = xDiff * weights.x + yDiff * weights.y + zDiff * weights.z;

	return height;
}

float3 SampleNormalMapWithTriMapping(Texture2D normalMap, float3 position, float3 normal, float3 textureScale, float sharpness = 15.0f) {
	float2 uvX = position.zy * textureScale.x;
	float2 uvY = position.xz * textureScale.y;
	float2 uvZ = position.xy * textureScale.z;

	float3 weights = pow(abs(normal), sharpness);
	weights /= dot(weights, 1.0f);

	float3 tnormalX = normalMap.SampleLevel(SamplerLinearWrap, uvX, 0).xyz * 2.0f - 1.0f;
    float3 tnormalY = normalMap.SampleLevel(SamplerLinearWrap, uvY, 0).xyz * 2.0f - 1.0f;
    float3 tnormalZ = normalMap.SampleLevel(SamplerLinearWrap, uvZ, 0).xyz * 2.0f - 1.0f;

	tnormalX = float3(
		tnormalX.xy + normal.zy,
		abs(tnormalX.z) * normal.x
		);
	tnormalY = float3(
		tnormalY.xy + normal.xz,
		abs(tnormalY.z) * normal.y
		);
	tnormalZ = float3(
		tnormalZ.xy + normal.xy,
		abs(tnormalZ.z) * normal.z
		);

	float3 wsNormal = normalize(
		tnormalX.zyx * weights.x +
		tnormalY.xzy * weights.y +
		tnormalZ.xyz * weights.z
	);

	return wsNormal;
}

float4 GetHeightBlend(float high1, float high2, float high3, float high4, float4 splatWeight) {
    float4 blend = float4(high1, high2, high3, high4) * splatWeight;
    float ma = max(blend.r, max(blend.g, max(blend.b, blend.a)));

    //与权重最大的通道进行对比，高度差在_Weight范围内的将会保留,_Weight不可以为0
    blend = max(blend - ma + 0.2f, 0.0f) * splatWeight;
    return blend / (blend.r + blend.g + blend.b + blend.a);
}

v2p VSMain(a2v input, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<DrawRvtTiledMapRequest> drawRequestBuffer = ResourceDescriptorHeap[PassDataCB.drawRequestBufferIndex];
	Texture2D<float4> heightMap = ResourceDescriptorHeap[PassDataCB.terrainHeightMapIndex];

	DrawRvtTiledMapRequest drawRequest = drawRequestBuffer[instanceID];
	float2 scale = drawRequest.tileRectInWorldSpace.zw;
	float3 wsPos = float3(0.0f, 0.0f, 0.0f);
	wsPos.xz = input.lsPos.xy * scale;
	wsPos.xz += drawRequest.tileRectInWorldSpace.xy;

	float2 heightUV = (wsPos.xz + (PassDataCB.terrainMeterSize * 0.5f) + 0.5f) / (PassDataCB.terrainMeterSize + 1.0f);
	heightUV *= 1.0f;
	float height = heightMap.SampleLevel(SamplerLinearWrap, heightUV, 0u).r;
	wsPos.y = height * PassDataCB.terrainHeightScale;

	v2p output;
	output.currCsPos = mul(float4(input.lsPos, 1.0f), drawRequest.mvpMatrix);
	output.wsPos = wsPos;
    output.heightUV = heightUV;

	return output;
}

p2o PSMain(v2p input) {
	Texture2D rChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.rChannelAlbedoMapIndex];
	Texture2D rChannelNormalMap = ResourceDescriptorHeap[PassDataCB.rChannelNormalMapIndex];
	Texture2D rChannelHeightMap = ResourceDescriptorHeap[PassDataCB.rChannelHeightMapIndex];

	Texture2D gChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.gChannelAlbedoMapIndex];
	Texture2D gChannelNormalMap = ResourceDescriptorHeap[PassDataCB.gChannelNormalMapIndex];
	Texture2D gChannelHeightMap = ResourceDescriptorHeap[PassDataCB.gChannelHeightMapIndex];

	Texture2D bChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.bChannelAlbedoMapIndex];
	Texture2D bChannelNormalMap = ResourceDescriptorHeap[PassDataCB.bChannelNormalMapIndex];
	Texture2D bChannelHeightMap = ResourceDescriptorHeap[PassDataCB.bChannelHeightMapIndex];

	Texture2D aChannelAlbedoMap = ResourceDescriptorHeap[PassDataCB.aChannelAlbedoMapIndex];
	Texture2D aChannelNormalMap = ResourceDescriptorHeap[PassDataCB.aChannelNormalMapIndex];
	Texture2D aChannelHeightMap = ResourceDescriptorHeap[PassDataCB.aChannelHeightMapIndex];

	float  textureScale = 1.0f / 64.0f;

	float3 wsPos = input.wsPos;
	float3 wsNormal = SampleTerrainNormalMap(input.heightUV);
	float4 splatWeight = SampleTerrainSplatMap(input.heightUV);

	float3 finalAlbedo = float3(0.0f, 0.0f, 0.0f);
	float3 finalNormal = float3(0.0f, 0.0f, 0.0f);

	// Height
	float rChannelHeight = SampleHeightMapWithTriMapping(rChannelHeightMap, wsPos, wsNormal, textureScale);
	float gChannelHeight = SampleHeightMapWithTriMapping(gChannelHeightMap, wsPos, wsNormal, textureScale);
	float bChannelHeight = SampleHeightMapWithTriMapping(bChannelHeightMap, wsPos, wsNormal, textureScale);
	float aChannelHeight = SampleHeightMapWithTriMapping(aChannelHeightMap, wsPos, wsNormal, textureScale);
	splatWeight = GetHeightBlend(rChannelHeight, gChannelHeight, bChannelHeight, aChannelHeight, splatWeight);

	// Albedo
	float3 rChannel = SampleColorMapWithTriMapping(rChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 gChannel = SampleColorMapWithTriMapping(gChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 bChannel = SampleColorMapWithTriMapping(bChannelAlbedoMap, wsPos, wsNormal, textureScale);
	float3 aChannel = SampleColorMapWithTriMapping(aChannelAlbedoMap, wsPos, wsNormal, textureScale);
	finalAlbedo.rgb = saturate(splatWeight.r * rChannel + splatWeight.g * gChannel + splatWeight.b * bChannel + splatWeight.a * aChannel);
	
	// Normal
	rChannel = SampleNormalMapWithTriMapping(rChannelNormalMap, wsPos, wsNormal, textureScale);
	gChannel = SampleNormalMapWithTriMapping(gChannelNormalMap, wsPos, wsNormal, textureScale);
	bChannel = SampleNormalMapWithTriMapping(bChannelNormalMap, wsPos, wsNormal, textureScale);
	aChannel = SampleNormalMapWithTriMapping(aChannelNormalMap, wsPos, wsNormal, textureScale);
	finalNormal.xyz = normalize(splatWeight.r * rChannel + splatWeight.g * gChannel + splatWeight.b * bChannel + splatWeight.a * aChannel);

	p2o output;
	output.albedo.rgba = float4(finalAlbedo, 0.0f);
	output.normal.xyzw = float4(finalNormal, 0.0f);
	return output;
}

#endif