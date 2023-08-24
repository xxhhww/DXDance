#ifndef _OceanRenderer__
#define _OceanRenderer__

#include "OceanHelper.hlsl"
#include "../Atmosphere/AtmosphereHelper.hlsl"
#include "../Atmosphere/Raymarching.hlsl"
#include "../Atmosphere/Scattering.hlsl"

struct PassData{
	WaterParameter parameter;
    // AtmosphereParameter atmosphereParameter;

    uint  transmittanceLutIndex;
    uint  skyViewLutIndex;
    float pad1;
    float pad2;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Base/BRDFHelper.hlsl"
#include "../Math/MathCommon.hlsl"

struct a2v {
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float4 color     : COLOR;
};

struct v2h{
	float4 lsPos     : POSITION;
	float4 uv        : TEXCOORD;
};

struct TessellationPatch{
	float edgeTess[3]   : SV_TESSFACTOR;
	float insideTess    : SV_INSIDETESSFACTOR;
};

struct h2d{
	float4 lsPos     : POSITION;
	float4 uv        : TEXCOORD;
};

struct d2p {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float4 positionView   : TEXCOORD0;
    float4 texCoord0      : TEXCOORD1;
    float4 screenPosition : TEXCOORD2;
    float4 positionWorld  : TEXCOORD3;
    float4 worldNormalAndHeight : TEXCOORD4;
};

struct p2o {
    float4 shadingResult   : SV_TARGET0;
	float2 screenVelocity  : SV_TARGET1;
};

v2h VSMain(a2v input) {
	v2h output;
    output.lsPos = float4(input.lsPos, 1.0f);
    output.uv = input.color;
 
    return output;
}

TessellationPatch WaterTessellation(InputPatch<v2h, 3> inputPatch, uint patchId : SV_PRIMITIVEID) {
	WaterParameter parameter = PassDataCB.parameter;

    TessellationPatch output;
    output.edgeTess[0] = output.edgeTess[1] = output.edgeTess[2] = parameter.tessellationFactor;
    output.insideTess = parameter.tessellationFactor;
 
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
    WaterParameter parameter = PassDataCB.parameter;

    d2p output;
    output.position = uvwCoord.x * patch[0].lsPos + uvwCoord.y * patch[1].lsPos + uvwCoord.z * patch[2].lsPos;
    output.texCoord0 = uvwCoord.x * patch[0].uv + uvwCoord.y * patch[1].uv + uvwCoord.z * patch[2].uv;
 
    Wave waves[numWaves];
    waves[0].direction = float3(0.3, 0, -0.7);
    waves[0].steepness = 1.79;
    waves[0].waveLength = 3.75;
    waves[0].amplitude = 0.85;
    waves[0].speed = 1.21;
 
    waves[1].direction = float3(0.5, 0, -0.2);
    waves[1].steepness = 1.79;
    waves[1].waveLength = 4.1;
    waves[1].amplitude = 0.52;
    waves[1].speed = 1.03;
 
    float dampening = 1.0 - pow(saturate(abs(output.texCoord0.z - 0.5) / 0.5), parameter.dampeningFactor);
    dampening *= 1.0 - pow(saturate(abs(output.texCoord0.w - 0.5) / 0.5), parameter.dampeningFactor);
 
    WaveResult finalWaveResult;
    finalWaveResult.position = float3(0,0,0);
    finalWaveResult.normal = float3(0,0,0);
    finalWaveResult.tangent = float3(0,0,0);
    finalWaveResult.binormal = float3(0,0,0);
 
    for(uint waveId = 0; waveId < numWaves; waveId++) {
        WaveResult waveResult = CalculateWave(waves[waveId], output.position.xyz, dampening, FrameDataCB.TotalTime);
        finalWaveResult.position += waveResult.position;
        finalWaveResult.normal += waveResult.normal;
        finalWaveResult.tangent += waveResult.tangent;
        finalWaveResult.binormal += waveResult.binormal;
    }
 
    finalWaveResult.position -= output.position.xyz * (numWaves - 1);
    finalWaveResult.normal = normalize(finalWaveResult.normal);
    finalWaveResult.tangent = normalize(finalWaveResult.tangent);
    finalWaveResult.binormal = normalize(finalWaveResult.binormal);
 
    output.worldNormalAndHeight.w = finalWaveResult.position.y - output.position.y;
    output.position = float4(finalWaveResult.position, 1.0);

    // output.positionWorld = mul(output.position, modelMatrix);
    output.positionWorld = output.position + float4(0.0f, 1700.0f, 0.0f, 0.0f);
    output.positionView = mul(output.positionWorld, FrameDataCB.CurrentEditorCamera.View);
    output.position = mul(output.positionView, FrameDataCB.CurrentEditorCamera.Projection);
    output.screenPosition = output.position;

    // output.normal = normalize(mul(finalWaveResult.normal, (float3x3)modelMatrix));
    output.normal = normalize(finalWaveResult.normal);
    output.worldNormalAndHeight.xyz = output.normal;
    output.normal = normalize(mul(float4(output.normal, 0.0), FrameDataCB.CurrentEditorCamera.View).xyz);
    // output.tangent = normalize(mul(finalWaveResult.tangent, (float3x3)modelMatrix));
    output.tangent = normalize(finalWaveResult.tangent);
    output.tangent = normalize(mul(float4(output.tangent, 0.0), FrameDataCB.CurrentEditorCamera.View).xyz);
    // output.binormal = normalize(mul(finalWaveResult.binormal, (float3x3)modelMatrix));
    output.binormal = normalize(finalWaveResult.binormal);
    output.binormal = normalize(mul(float4(output.binormal, 0.0), FrameDataCB.CurrentEditorCamera.View).xyz);

    return output;
}

p2o PSMain(d2p input) {
    WaterParameter parameter = PassDataCB.parameter;

    Texture2D waterNormalMap1 = ResourceDescriptorHeap[parameter.waterNormalMap1Index];
    Texture2D waterNormalMap2 = ResourceDescriptorHeap[parameter.waterNormalMap2Index];
    Texture2D waterFoamMap = ResourceDescriptorHeap[parameter.waterFoamMapIndex];
    Texture2D waterNoiseMap = ResourceDescriptorHeap[parameter.waterNoiseMapIndex];

    Texture2D<float4> skyViewLut = ResourceDescriptorHeap[PassDataCB.skyViewLutIndex];
    Texture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];

    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    input.binormal = normalize(input.binormal);
 
    float2 normalMapCoords1 = input.texCoord0.xy + FrameDataCB.TotalTime * parameter.normalMapScroll.xy * parameter.normalMapScrollSpeed.x;
    float2 normalMapCoords2 = input.texCoord0.xy + FrameDataCB.TotalTime * parameter.normalMapScroll.zw * parameter.normalMapScrollSpeed.y;
    // float2 hdrCoords = ((float2(input.screenPosition.x, -input.screenPosition.y) / input.screenPosition.w) * 0.5) + 0.5;
 
    float3 normalMap1 = (waterNormalMap1.SampleLevel(SamplerLinearWrap, normalMapCoords1, 0).rgb * 2.0f) - 1.0f;
    float3 normalMap2 = (waterNormalMap2.SampleLevel(SamplerLinearWrap, normalMapCoords2, 0).rgb * 2.0f) - 1.0f;
    float3x3 texSpace = float3x3(input.tangent, input.binormal, input.normal);
    float3 finalNormal = normalize(mul(normalMap1.xyz, texSpace));
    finalNormal += normalize(mul(normalMap2.xyz, texSpace));
    finalNormal = normalize(finalNormal);

    // view space
    Light sunLight = LightDataSB[0];
    float3 lightDir = normalize(mul(float4(sunLight.position.xyz, 0.0f), FrameDataCB.CurrentEditorCamera.View).xyz);
    float3 viewDir = -normalize(input.positionView.xyz);
    float  roughness = parameter.roughness;
    float  reflectance = parameter.reflectance;

    float3 half  = normalize(viewDir + lightDir);
    float  nDotL = saturate(dot(finalNormal, lightDir));
    float  nDotV = abs(dot(finalNormal, viewDir));
    float  nDotH = saturate(dot(finalNormal, half));
    float  lDotH = saturate(dot(lightDir, half));
    
    float3 f0 = 0.16 * reflectance * reflectance;
    float  normalDistribution = distributionGGX(nDotH, roughness);
    float3 fresnelReflectance = fresnelSchlick(lDotH, f0);
    float  geometryTerm = geometrySmith(nDotL, nDotV, roughness);
    
    float  specularNoise = waterNoiseMap.SampleLevel(SamplerLinearWrap, normalMapCoords1 * 0.5, 0).r;
    specularNoise *= waterNoiseMap.SampleLevel(SamplerLinearWrap, normalMapCoords2 * 0.5, 0).r;
    specularNoise *= waterNoiseMap.SampleLevel(SamplerLinearWrap, input.texCoord0.xy * 0.5, 0).r;
    
    float3 specularFactor = (geometryTerm * normalDistribution) * fresnelReflectance * parameter.specIntensity * nDotL * specularNoise;

    float  sceneZ = 0;
    float  stepCount = 0;
    float  forwardStepCount = parameter.ssrSettings.y;
    float3 rayMarchPosition = input.positionView.xyz;
    float4 rayMarchTexPosition = float4(0,0,0,0);
    float3 reflectionVector = normalize(reflect(-viewDir, finalNormal));

    // float3 reflectionColor = HDRMap.Sample(SamplerPointClamp, rayMarchTexPosition.xy).rgb;
    float3 envReflection = mul(reflectionVector, (float3x3)FrameDataCB.CurrentEditorCamera.InverseView);
    
    float3 skyboxColor = skyViewLut.SampleLevel(SamplerLinearClamp, ViewDirToUV(envReflection), 0).rgb;
    
    float3 reflectionColor = skyboxColor * parameter.waterSurfaceColor.rgb;
    
    /*
    float2 distortedTexCoord = (hdrCoords + ((finalNormal.xz + finalNormal.xy) * 0.5) * refractionDistortionFactor);
    float  distortedDepth = DepthMap.Sample(PointClampSampler, distortedTexCoord).r;
    float3 distortedPosition = GetWorldPositionFromDepth(distortedTexCoord, distortedDepth, viewProjInvMatrix);
    float2 refractionTexCoord = (distortedPosition.y < input.positionWorld.y) ? distortedTexCoord : hdrCoords;
    float3 waterColor = HDRMap.Sample(PointClampSampler, refractionTexCoord).rgb * waterRefractionColor.rgb;
    
    float  sceneDepth = DepthMap.Sample(PointClampSampler, hdrCoords).r;
    float3 scenePosition = GetWorldPositionFromDepth(hdrCoords, sceneDepth, viewProjInvMatrix);
    float  depthSoftenedAlpha = saturate(distance(scenePosition, input.positionWorld.xyz) / depthSofteningDistance);
    
    float3 waterSurfacePosition = (distortedPosition.y < input.positionWorld.y) ? distortedPosition : scenePosition;
    waterColor = lerp(waterColor, waterRefractionColor.rgb, saturate((input.positionWorld.y - waterSurfacePosition.y) / refractionHeightFactor));

    float  waveTopReflectionFactor = pow(1.0 - saturate(dot(input.normal, viewDir)), 3);
    float3 waterBaseColor = lerp(waterColor, reflectionColor, saturate(saturate(length(input.positionView.xyz) / refractionDistanceFactor) + waveTopReflectionFactor));
    float3 finalWaterColor = waterBaseColor + specularFactor;
    */
    float3 finalWaterColor = specularFactor;

    p2o output;
    output.shadingResult = float4(finalWaterColor, 1.0f);
    output.screenVelocity = float2(0.0f, 0.0f);


    return output;
}

#endif