#ifndef _OceanRenderer__
#define _OceanRenderer__

#include "OceanHelper.hlsl"

struct PassData{
	float4 oceanColorShallow;
	float4 oceanColorDeep;
	float4 bubblesColor;
	float4 specular;

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

    // Displacement
    float3 displace = displaceMap.SampleLevel(SamplerLinearWrap, uv, 0u).xyz;
    position += displace;

    // ModelMatrix
    position += float3(0.0f, 1700.0f, 0.0f);

    d2p output;
    output.uv = uv;
    output.wsPos = position;
    output.currCsPos = mul(float4(output.wsPos, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjection);

    return output;
}

p2o PSMain(d2p input) {
    Texture2D<float4> oceanNormalMap = ResourceDescriptorHeap[PassDataCB.oceanNormalMapIndex];
	Texture2D<float4> oceanBubblesMapIndex = ResourceDescriptorHeap[PassDataCB.oceanBubblesMapIndex];
    Texture2D<float4> skyViewLutIndex = ResourceDescriptorHeap[PassDataCB.skyViewLutIndex];

    p2o output;
    output.shadingResult = float4(0.5f, 0.5f, 0.5f, 1.0f);
    output.screenVelocity = float2(0.0f, 0.0f);


    return output;
}

#endif