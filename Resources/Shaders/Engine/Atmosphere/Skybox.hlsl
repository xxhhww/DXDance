#ifndef _Skybox__
#define _Skybox__

#include "AtmosphereHelper.hlsl"
#include "Raymarching.hlsl"
#include "Scattering.hlsl"

struct PassData{
    AtmosphereParameter parameter;

    uint transmittanceLutIndex;
    uint multiScatteringLutIndex;
    uint skyViewLutIndex;
    uint aerialPerspectiveLutIndex;
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
	float3 wsPos     : POSITION0;
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
};

float3 GetSunDisk(in AtmosphereParameter param, float3 eyePos, float3 viewDir, float3 lightDir) {
    Texture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];

    // 计算入射光照
    float cosine_theta = dot(viewDir, -lightDir);
    float theta = acos(cosine_theta) * (180.0 / PI);
    float3 sunLuminance = param.SunLightColor * param.SunLightIntensity;

    // 判断光线是否被星球阻挡
    float disToPlanet = RayIntersectSphere(float3(0,0,0), param.PlanetRadius, eyePos, viewDir);
    if(disToPlanet >= 0) return float3(0,0,0);

    // 和大气层求交
    float disToAtmosphere = RayIntersectSphere(float3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir);
    if(disToAtmosphere < 0) return float3(0,0,0);

    // 计算衰减
    //float3 hitPoint = eyePos + viewDir * disToAtmosphere;
    //sunLuminance *= Transmittance(param, hitPoint, eyePos);
    sunLuminance *= TransmittanceToAtmosphere(param, eyePos, viewDir, transmittanceLut, SamplerLinearClamp);

    if(theta < param.SunDiskAngle) return sunLuminance;
    return float3(0,0,0);
}

v2p VSMain(a2v input) {
	v2p output;
    output.currCsPos = mul(float4(input.lsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);
    output.wsPos = normalize(input.lsPos);

    return output;
}

p2o PSMain(v2p input) {
    Texture2D<float4> skyViewLut = ResourceDescriptorHeap[PassDataCB.skyViewLutIndex];

    AtmosphereParameter param = PassDataCB.parameter;

    float4 color = float4(0, 0, 0, 1);
    float3 viewDir = normalize(input.wsPos);

    Light sunLight = LightDataSB[0];
    float3 lightDir = -sunLight.position.xyz;

    float3 cameraPos = FrameDataCB.CurrentEditorCamera.Position.xyz;
    float h = cameraPos.y - param.SeaLevel + param.PlanetRadius;
    float3 eyePos = float3(0, h, 0);
    
    color.rgb += skyViewLut.SampleLevel(SamplerLinearClamp, ViewDirToUV(viewDir), 0).rgb;
    color.rgb += GetSunDisk(param, eyePos, viewDir, lightDir);

    p2o output;
    output.shadingResult = color;
    return output;
}

#endif