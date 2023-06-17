#ifndef _Shading__
#define _Shading__

struct ShadingResult {
    uint4 RayLightIntersectionData;
    float3 AnalyticUnshadowedOutgoingLuminance;
    float4 RayPDFs;
    float4x3 StochasticUnshadowedOutgoingLuminance;
};

/*
* Ì«Ñô¹â
*/
void ShadeWithSunLight()

#endif