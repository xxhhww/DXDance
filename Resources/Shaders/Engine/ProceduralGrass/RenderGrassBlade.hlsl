#ifndef _RenderGrassBlade__
#define _RenderGrassBlade__

#include "ProceduralGrassHelper.hlsl"
#include "../Base/Light.hlsl"

struct PassData {
	uint bakedGrassBladeListIndex;
	uint visibleGrassBladeIndexListIndex;
	uint grassAlbedoMapIndex;
	uint grassNormalMapIndex;

	uint currLODIndex;			// LOD0 / LOD1
	uint  grassVertexBufferIndex;
	uint  grassIndexBufferIndex;
	float pad1;

	float p1Flexibility;
	float p2Flexibility;
	float waveAmplitude;
	float waveSpeed;

	float wavePower;
	float sinOffsetRange;
	float pushTipOscillationForward;
	float widthTaperAmount;
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
	StructuredBuffer<GrassBlade>           bakedGrassBladeList = ResourceDescriptorHeap[PassDataCB.bakedGrassBladeListIndex];
	StructuredBuffer<uint>                 visibleGrassBladeIndexList = ResourceDescriptorHeap[PassDataCB.visibleGrassBladeIndexListIndex];
	StructuredBuffer<uint>                 grassIndexBuffer = ResourceDescriptorHeap[PassDataCB.grassIndexBufferIndex];
	StructuredBuffer<GrassVertexAttribute> grassVertexBuffer = ResourceDescriptorHeap[PassDataCB.grassVertexBufferIndex];

	v2p output;

	// Extract vertex position and color from mesh
    // NOTE: I think we are going through each vertex multiple times by going over each triangle index in the mesh
    // Try go over individual vertices instead!
	int vertexIndex = grassIndexBuffer[vertexID];
	GrassVertexAttribute grassVertexAttribute = grassVertexBuffer[vertexIndex];

	// Get the t and side information from the vertex color
    float t = grassVertexAttribute.t;
    float side = grassVertexAttribute.side;
    side = (side * 2.0f) - 1.0f;

	// Get the blade attribute data calculated in the compute shader
    GrassBlade grassBlade = bakedGrassBladeList[visibleGrassBladeIndexList[instanceID]];
    float3 bladePosition = grassBlade.position;
    float2 bladeFacing   = grassBlade.facing;
	float  height        = grassBlade.height;
    float  tilt          = grassBlade.tilt;         // 控制草叶的倾斜
    float  bend          = grassBlade.bend;         // 控制草叶的弯曲(其实就是控制贝塞尔样条曲线)
    float  mult          = 1.0f - bend;               
    float  hash          = grassBlade.hash;
    float  sideCurve     = grassBlade.sideCurve;    // 控制草叶的边的弯曲
    // float  windStrength  = grassBlade.windStrength;
    float  windStrength = 0.3f;

    // Calculate p0, p1, p2, p3 for the spline
    float3 p0 = float3(0.0f, 0.0f, 0.0f);

    float  p3y = tilt * height;						// 对高度施加倾斜状态(更改Y值)
    float  p3x = sqrt(height * height - p3y * p3y);	// 计算倾斜后的X值
    float3 p3  = float3(-p3x, p3y, 0.0f);			// 贝塞尔曲线的P3控制点

    // NOTE: Change this to more efficient. Work in only the x,y plane, ignore z
    float3 grassBladeDir = normalize(p3);			// 草叶的倾斜方向(XY平面)
    float3 bezierCtrlDir = normalize(cross(grassBladeDir, float3(0.0f, 0.0f, 1.0f)));	// 贝塞尔控制方向

    float3 p1 = 0.33f * p3;	// 贝塞尔曲线的P1控制点(可控)
    float3 p2 = 0.66f * p3;	// 贝塞尔曲线的P2控制点(可控)

    p1 += bezierCtrlDir * bend * PassDataCB.p1Flexibility;
    p2 += bezierCtrlDir * bend * PassDataCB.p2Flexibility;

    float p1Weight = 0.33f;
    float p2Weight = 0.66f;
    float p3Weight = 1.00f;

    float p1ffset = pow(p1Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((FrameDataCB.TotalTime / 20.0f + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p1Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength; 
    float p2ffset = pow(p2Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((FrameDataCB.TotalTime / 20.0f + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p2Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength;
    float p3ffset = pow(p3Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((FrameDataCB.TotalTime / 20.0f + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p3Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength; 
    p3ffset = (p3ffset) - PassDataCB.pushTipOscillationForward * mult * (pow(p3Weight, PassDataCB.wavePower) * PassDataCB.waveAmplitude / 100.0f) / 2.0f;

    p1 += bezierCtrlDir * p1ffset;
    p2 += bezierCtrlDir * p2ffset;
    p3 += bezierCtrlDir * p3ffset;

    // Evaluate Bezier curve(ignore Z, only XY平面)
    float3 midPoint         = cubicBezier(p0, p1, p2, p3, t);                       // 贝塞尔样条上的点(midPoint是因为生成该点所基于的样条正是叶片的中轴线)
    float3 midPointTangent  = normalize(bezierTangent(p0, p1, p2, p3, t));          // 该点的切线
    float3 midPointNormal   = normalize(cross(midPointTangent, float3(0.0f, 0.0f, 1.0f)));  // 该点的法线

    // 计算当前点处叶片的宽度
    float  currbladeWidth = grassBlade.width * (1.0f - PassDataCB.widthTaperAmount * t);    // 随着t的增加(也就是更加接近叶尖)，叶片的宽度应该减小
    // 计算叶片边上的点
    float3 sidePoint = float3(midPoint.x, midPoint.y, midPoint.z + currbladeWidth * side);
    // 计算该点的法线
    float3 sidePointNormal = midPointNormal;
    sidePointNormal.z += currbladeWidth * side;
    sidePointNormal = normalize(sidePointNormal);

    float angle = atan2(bladeFacing.y, bladeFacing.x);

    float3x3 facingRotateMat = AngleAxis3x3(-angle, float3(0.0f, 1.0f, 0.0f));
    float3x3 sideRotateMat   = AngleAxis3x3(sideCurve, normalize(midPointTangent));

    // 施加叶片的边的旋转
    float3 vectorFromMidToSide = sidePoint - midPoint;
    vectorFromMidToSide = mul(sideRotateMat, vectorFromMidToSide);
    sidePoint = midPoint + vectorFromMidToSide;

    midPointNormal  = mul(sideRotateMat, midPointNormal);
    sidePointNormal = mul(sideRotateMat, sidePointNormal);
    
    // 施加Facing旋转
    sidePoint       = mul(facingRotateMat, sidePoint);
    midPointNormal  = mul(facingRotateMat, midPointNormal);
    sidePointNormal = mul(facingRotateMat, sidePointNormal);

    // 移动到对应的世界坐标上
    float3 finalPosition = sidePoint + bladePosition;

    output.currCsPos = mul(float4(finalPosition, 1.0f), FrameDataCB.CurrentEditorCamera.ViewProjectionJitter);
    output.prevCsPos = mul(float4(finalPosition, 1.0f), FrameDataCB.PreviousEditorCamera.ViewProjection);
    output.wsPos = finalPosition;
    output.uv = grassVertexAttribute.uv0;
    output.normal = sidePointNormal;

    return output;
}

p2o PSMain(v2p input) {
    Texture2D<float4> grassAlbedoMap = ResourceDescriptorHeap[PassDataCB.grassAlbedoMapIndex];
    Texture2D<float4> grassNormalMap = ResourceDescriptorHeap[PassDataCB.grassNormalMapIndex];

    float4 grassAlbedo = grassAlbedoMap.SampleLevel(SamplerLinearWrap, input.uv, 0.0f);
    float4 grassNormal = grassNormalMap.SampleLevel(SamplerLinearWrap, input.uv, 0.0f);

    // 当前帧的uv抖动
	float2 uvJitter = FrameDataCB.CurrentEditorCamera.UVJitter;
    float3 prevNDCPos = input.prevCsPos.xyz / input.prevCsPos.w;
    float2 prevScreenUV = NDCToUV(prevNDCPos);
    prevScreenUV += uvJitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 prevUVSpacePos = float3(prevScreenUV, prevNDCPos.z);

    float2 currScreenUV = (floor(input.currCsPos.xy) + 0.5f) * FrameDataCB.FinalRTResolutionInv;
    float3 currUVSpacePos = float3(currScreenUV, input.currCsPos.z);

    float3 velocity = currUVSpacePos - prevUVSpacePos;

	Surface surface;
	surface.albedo = grassAlbedo.rgba;
	surface.normal = input.normal;
	surface.roughness = 0.99f;
	surface.metallic = 0.0f;
	surface.emission = 0.0f;

	surface.position = input.wsPos;
	float3 camToP = surface.position - FrameDataCB.CurrentEditorCamera.Position.xyz;
	surface.viewDir = -normalize(camToP);

	surface.InferRemainingProperties();

	LightContribution totalLighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	Light sunLight = LightDataSB[0];

	totalLighting.addSunLight(surface, sunLight/*, screenUV, pixelDepth,
		shadowMap, shadowSampler, lighting.shadowMapTexelSize, sssTexture, clampSampler*/);

    p2o output;
	output.shadingResult   = totalLighting.evaluate(surface.albedo);
	output.normalRoughness = float4(input.normal, 0.99f);
	output.screenVelocity  = float2(velocity.xy);

    return output;
}

#endif