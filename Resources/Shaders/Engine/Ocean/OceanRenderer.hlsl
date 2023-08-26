#ifndef _OceanRenderer__
#define _OceanRenderer__

#include "OceanHelper.hlsl"
#include "../Base/Light.hlsl"
#include "../Atmosphere/AtmosphereHelper.hlsl"

struct PassData{
	float4x4 modelTransform;
    
    float4 position;	// x,y,z : position; w : scale

	float4 oceanColorShallow;
	float4 oceanColorDeep;
	float4 bubblesColor;
	float4 specularColor;

	float gloss;
	float fresnelScale;
	float tessellationFactor;
	uint  displaceMapIndex;

	uint  oceanNormalMapIndex;
	uint  oceanBubblesMapIndex;
    uint  skyViewLutIndex;
	float pad1;
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

struct v2h {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
};

struct TessellationPatch {
	float edgeTess[3]   : SV_TESSFACTOR;
	float insideTess    : SV_INSIDETESSFACTOR;
};

struct h2d {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
};

struct d2p {
	float4 currCsPos : SV_POSITION;
	float4 prevCsPos : POSITION1;
    float3 wsPos     : POSITION2;
    float2 uv        : TEXCOORD0;
};

struct p2o {
    float4 shadingResult   : SV_TARGET0;
	float2 screenVelocity  : SV_TARGET1;
};

v2h VSMain(a2v input) {
    /*
    d2p output;
    output.uv = input.uv;
    output.wsPos = mul(float4(input.lsPos, 1.0f), PassDataCB.modelTransform).xyz;
    output.currCsPos = mul(float4(output.wsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);

    return output;
    */

	v2h output;
    output.lsPos = input.lsPos;
    output.uv = input.uv;
 
    return output;
}

TessellationPatch WaterTessellation(InputPatch<v2h, 3> inputPatch, uint patchId : SV_PRIMITIVEID) {
    TessellationPatch output;
    output.edgeTess[0] = output.edgeTess[1] = output.edgeTess[2] = PassDataCB.tessellationFactor;
    output.insideTess = PassDataCB.tessellationFactor;
 
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(15.0)]
[patchconstantfunc("WaterTessellation")]
h2d HSMain(InputPatch<v2h, 3> inputPatch, uint pointId : SV_OUTPUTCONTROLPOINTID, uint patchId : SV_PRIMITIVEID) {
    h2d output;
    output.lsPos = inputPatch[pointId].lsPos;
    output.uv = inputPatch[pointId].uv;
 
    return output;
}


[domain("tri")]
d2p DSMain(TessellationPatch input, float3 uvwCoord : SV_DOMAINLOCATION, const OutputPatch<h2d, 3> patch) {
    Texture2D<float4> displaceMap = ResourceDescriptorHeap[PassDataCB.displaceMapIndex];
    
    float3 position = uvwCoord.x * patch[0].lsPos + uvwCoord.y * patch[1].lsPos + uvwCoord.z * patch[2].lsPos;
    float2 uv = uvwCoord.x * patch[0].uv + uvwCoord.y * patch[1].uv + uvwCoord.z * patch[2].uv;

    float3 displcae = displaceMap.SampleLevel(SamplerLinearWrap, uv, 0).xyz;
    position += displcae;

    d2p output;
    output.uv = uv;
    output.wsPos = mul(float4(position, 1.0f), PassDataCB.modelTransform).xyz;
    output.currCsPos = mul(float4(output.wsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);

    return output;
}

p2o PSMain(d2p input) {
    Texture2D<float4> oceanNormalMap = ResourceDescriptorHeap[PassDataCB.oceanNormalMapIndex];
	Texture2D<float4> oceanBubblesMap = ResourceDescriptorHeap[PassDataCB.oceanBubblesMapIndex];
    Texture2D<float4> skyViewLut = ResourceDescriptorHeap[PassDataCB.skyViewLutIndex];

    float3 normal = normalize(oceanNormalMap.SampleLevel(SamplerLinearWrap, input.uv, 0).xyz);
    // normal = mul(float4(normal, 0.0f), PassDataCB.modelTransform).xyz;
    float  bubbles = oceanBubblesMap.SampleLevel(SamplerLinearWrap, input.uv, 0).x;

    Light sunLight = LightDataSB[0];
    float3 lightDir = normalize(sunLight.position.xyz);
    float3 surfacePos = input.wsPos;
    float3 viewDir = normalize(FrameDataCB.CurrentEditorCamera.Position.xyz - surfacePos);
    float3 reflectionDir = normalize(reflect(-viewDir, normal));

    float  fresnel = saturate(PassDataCB.fresnelScale + (1 - PassDataCB.fresnelScale) * pow(1 - dot(normal, viewDir), 5));
    float  facing = saturate(dot(viewDir, normal));                
    float3 oceanColor = lerp(PassDataCB.oceanColorShallow, PassDataCB.oceanColorDeep, facing).rgb;
    float3 bubblesColor = PassDataCB.bubblesColor.rgb;

    // 计算海沫颜色
    Surface surface;

	surface.albedo = float4(bubblesColor, 1.0f);
	surface.normal = normal;
	surface.roughness = 0.99f;
	surface.metallic = 0.0f;
	surface.emission = 0.0f;
	surface.position = surfacePos;
	surface.viewDir = viewDir;
    surface.InferRemainingProperties();

    LightContribution totalLighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };
	totalLighting.addSunLight(surface, sunLight/*, screenUV, pixelDepth,
		shadowMap, shadowSampler, lighting.shadowMapTexelSize, sssTexture, clampSampler*/);
    float3 bubblesDiffuse = bubblesColor * totalLighting.diffuse;

    // 计算海洋颜色
    surface.albedo = float4(oceanColor, 1.0f);
	surface.roughness = 0.1f;
    surface.InferRemainingProperties();

    totalLighting.diffuse = float3(0.f, 0.f, 0.f); 
    totalLighting.specular = float3(0.f, 0.f, 0.f);
	totalLighting.addSunLight(surface, sunLight/*, screenUV, pixelDepth,
		shadowMap, shadowSampler, lighting.shadowMapTexelSize, sssTexture, clampSampler*/);
    float3 oceanDiffuse = oceanColor * totalLighting.diffuse;
    float3 oceanSpecular = PassDataCB.specularColor.rgb * totalLighting.specular * 10.0f;
    
    float3 diffuse = lerp(oceanDiffuse, bubblesDiffuse, bubbles);

    // 天空盒反射
    float3 skyColor = skyViewLut.SampleLevel(SamplerLinearClamp, ViewDirToUV(reflectionDir), 0).rgb;

    float3 col = lerp(diffuse, skyColor * 0.5f, fresnel) + oceanSpecular;
    // float3 col = diffuse + oceanSpecular;

    p2o output;
    output.shadingResult = float4(col, 1.0f);
    output.screenVelocity = float2(0.0f, 0.0f);


    return output;
}

#endif