#ifndef _Atmosphere__
#define _Atmosphere__

#include "Scattering.hlsl"
#include "Raymarching.hlsl"
#include "AtmosphereHelper.hlsl"

struct PassData {
	AtmosphereParameter parameter;

    uint transmittanceLutIndex;
    float2 transmittanceLutSize;
    float pad1;

    uint multiScatteringLutIndex;
    float2 multiScatteringLutSize;
    float pad2;

    uint skyViewLutIndex;
    float2 skyViewLutSize;
    float pad3;

    uint aerialPerspectiveLutIndex;
    float2 aerialPerspectiveLutSize; // 32 * 32, 32(Z平铺了，不做Texture3D)
    float pad4;

    float3 aerialPerspectiveVoxelSize;
    float aerialPerspectiveDistance;
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"
#include "../Base/Utils.hlsl"
#include "../Base/Light.hlsl"

[numthreads(8, 8, 1)]
void TransmittanceLut(uint3 dispatchThreadID : SV_DispatchThreadID) {
    RWTexture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];

    AtmosphereParameter param = PassDataCB.parameter;

    uint2 pixelIndex = dispatchThreadID.xy;
    float4 color = float4(0, 0, 0, 1);
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.transmittanceLutSize);

    float bottomRadius = param.PlanetRadius;
    float topRadius = param.PlanetRadius + param.AtmosphereHeight;

    // 计算当前 uv 对应的 cos_theta, height
    float cos_theta = 0.0;
    float r = 0.0;
    UvToTransmittanceLutParams(bottomRadius, topRadius, uv, cos_theta, r);

    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float3 viewDir = float3(sin_theta, cos_theta, 0);
    float3 eyePos = float3(0, r, 0);

    // 光线和大气层求交
    float dis = RayIntersectSphere(float3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir);
    float3 hitPoint = eyePos + viewDir * dis;

    // raymarch 计算 transmittance
    color.rgb = Transmittance(param, eyePos, hitPoint);

    transmittanceLut[pixelIndex] = color;
}

[numthreads(8, 8, 1)]
void MultiScatteringLut(uint3 dispatchThreadID : SV_DispatchThreadID) {
    Texture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];
    RWTexture2D<float4> multiScatteringLut = ResourceDescriptorHeap[PassDataCB.multiScatteringLutIndex];

    AtmosphereParameter param = PassDataCB.parameter;

    uint2 pixelIndex = dispatchThreadID.xy;
    float4 color = float4(0, 0, 0, 1);
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.multiScatteringLutSize);

    float mu_s = uv.x * 2.0 - 1.0;
    float r = uv.y * param.AtmosphereHeight + param.PlanetRadius;

    float cos_theta = mu_s;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float3 lightDir = float3(sin_theta, cos_theta, 0);
    float3 p = float3(0, r, 0);

    color.rgb = IntegralMultiScattering(param, p, lightDir, transmittanceLut, SamplerLinearClamp);

    multiScatteringLut[pixelIndex] = color;
}

[numthreads(8, 8, 1)]
void SkyViewLut(uint3 dispatchThreadID : SV_DispatchThreadID) {
    Texture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];
    Texture2D<float4> multiScatteringLut = ResourceDescriptorHeap[PassDataCB.multiScatteringLutIndex];
    RWTexture2D<float4> skyViewLut = ResourceDescriptorHeap[PassDataCB.skyViewLutIndex];

    AtmosphereParameter param = PassDataCB.parameter;

    uint2 pixelIndex = dispatchThreadID.xy;
    float4 color = float4(0, 0, 0, 1);
    float2 uv = TexelIndexToUV(pixelIndex, PassDataCB.skyViewLutSize);
    float3 viewDir = UVToViewDir(uv);

    // Get Sun Direction
    Light sunLight = LightDataSB[0];
    float3 lightDir = sunLight.position.xyz;
    
    float3 cameraPos = FrameDataCB.CurrentEditorCamera.Position.xyz;
    float h = cameraPos.y - param.SeaLevel + param.PlanetRadius;
    float3 eyePos = float3(0, h, 0);

    color.rgb = GetSkyView(
        param, eyePos, viewDir, lightDir, -1.0f,
        transmittanceLut, multiScatteringLut, SamplerLinearClamp
    );

    skyViewLut[pixelIndex] = color;
}

[numthreads(8, 8, 1)]
// 32 * 32, 32
void AerialPerspectiveLut(uint3 dispatchThreadID : SV_DispatchThreadID){
    Texture2D<float4> transmittanceLut = ResourceDescriptorHeap[PassDataCB.transmittanceLutIndex];
    Texture2D<float4> multiScatteringLut = ResourceDescriptorHeap[PassDataCB.multiScatteringLutIndex];
    RWTexture2D<float4> aerialPerspectiveLut = ResourceDescriptorHeap[PassDataCB.aerialPerspectiveLutIndex];

    AtmosphereParameter param = PassDataCB.parameter;

    uint2 pixelIndex = dispatchThreadID.xy;
    float4 color = float4(0, 0, 0, 1);
    float3 uv = float3(TexelIndexToUV(pixelIndex, PassDataCB.aerialPerspectiveLutSize), 0);
    uv.x *= PassDataCB.aerialPerspectiveVoxelSize.x * PassDataCB.aerialPerspectiveVoxelSize.z;  // X * Z
    uv.z = int(uv.x / PassDataCB.aerialPerspectiveVoxelSize.z) / PassDataCB.aerialPerspectiveVoxelSize.x;
    uv.x = fmod(uv.x, PassDataCB.aerialPerspectiveVoxelSize.z) / PassDataCB.aerialPerspectiveVoxelSize.x;
    uv.xyz += 0.5 / PassDataCB.aerialPerspectiveVoxelSize.xyz;

    float aspect = FrameDataCB.CurrentEditorCamera.AspectRatio;
    /*
    float3 viewDir = normalize(mul(unity_CameraToWorld, float4(
        (uv.x * 2.0 - 1.0) * 1.0, 
        (uv.y * 2.0 - 1.0) / aspect, 
        1.0, 0.0
    )).xyz);
    */
    float3 viewDir = normalize(mul(float4(
        (uv.x * 2.0 - 1.0) * 1.0, 
        (uv.y * 2.0 - 1.0) / aspect, 
        1.0, 0.0), FrameDataCB.CurrentEditorCamera.InverseView).xyz);

    // Get Sun Direction
    Light sunLight = LightDataSB[0];
    float3 lightDir = sunLight.position.xyz;

    float3 cameraPos = FrameDataCB.CurrentEditorCamera.Position.xyz;
    float h = cameraPos.y - param.SeaLevel + param.PlanetRadius;
    float3 eyePos = float3(0, h, 0);

    float maxDis = uv.z * PassDataCB.aerialPerspectiveDistance;

    // inScattering
    color.rgb = GetSkyView(
        param, eyePos, viewDir, lightDir, maxDis,
        transmittanceLut, multiScatteringLut, SamplerLinearClamp
    );

    // transmittance
    float3 voxelPos = eyePos + viewDir * maxDis;
    float3 t1 = TransmittanceToAtmosphere(param, eyePos, viewDir, transmittanceLut, SamplerLinearClamp);
    float3 t2 = TransmittanceToAtmosphere(param, voxelPos, viewDir, transmittanceLut, SamplerLinearClamp);
    float3 t = t1 / t2;
    color.a = dot(t, float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));

    aerialPerspectiveLut[pixelIndex] = color;
}

#endif