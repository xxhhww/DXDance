#ifndef _GrassRenderer__
#define _GrassRenderer__

#include "GrassHelper.hlsl"

struct PassData {
	uint grassBladeBufferIndex0;
	uint grassBladeBufferIndex1;
	uint grassMeshIndicesBufferIndex;
	uint grassMeshVerticesBufferIndex;
    
	float p1Flexibility;
	float p2Flexibility;
	float waveAmplitude;
	float waveSpeed;

	float wavePower;
	float sinOffsetRange;
	float pushTipOscillationForward;
    float widthTaperAmount;             // ���Ʋ�Ҷ������Ų�Ҷ��ϸ
}

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

struct v2p {
	float4 currCsPos : SV_POSITION;
};

struct p2o {
	float4 shadingResult   : SV_TARGET0;
	float4 normalRoughness : SV_TARGET1;
	float2 screenVelocity  : SV_TARGET2;
};

v2p VSMain(uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) {
	StructuredBuffer<GrassBlade>  grassBladeBuffer0       = ResourceDescriptorHeap[PassDataCB.grassBladeBufferIndex0];
	StructuredBuffer<int>         grassMeshIndicesBuffer  = ResourceDescriptorHeap[PassDataCB.grassMeshIndicesBufferIndex];
	StructuredBuffer<GrassVertex> grassMeshVerticesBuffer = ResourceDescriptorHeap[PassDataCB.grassMeshVerticesBufferIndex];

	v2p output;
	
	// Extract vertex position and color from mesh
    // NOTE: I think we are going through each vertex multiple times by going over each triangle index in the mesh
    // Try go over individual vertices instead!
	int vertexIndex = grassMeshIndicesBuffer[vertexID];
	GrassVertex grassVertex = grassMeshVerticesBuffer[vertexIndex];

	// Get the t and side information from the vertex color
    float t = grassVertex.color.r;
    float side = grassVertex.color.g;
    side = (side * 2.0f) - 1.0f;

	// Get the blade attribute data calculated in the compute shader
    GrassBlade grassBlade = grassBladeBuffer0[instanceID];
    float3 bladePosition = grassBlade.position;
	float  height        = grassBlade.height;
    float  tilt          = grassBlade.tilt;           // ���Ʋ�Ҷ����б
    float  bend          = grassBlade.bend;           // ���Ʋ�Ҷ������(��ʵ���ǿ��Ʊ�������������)
    float  mult          = 1.0f - bend;               
    float  hash          = grassBlade.hash;
    float  sideCurve     = blade.sideCurve;           // ���Ʋ�Ҷ�ıߵ�����
    float  windStrength  = grassBlade.windStrength;

    // Calculate p0, p1, p2, p3 for the spline
    float3 p0 = float3(0.0f, 0.0f, 0.0f);

    float  p3y = tilt * height;						// �Ը߶�ʩ����б״̬(����Yֵ)
    float  p3x = sqrt(height * height - p3y * p3y);	// ������б���Xֵ
    float3 p3  = float3(-p3x, p3y, 0.0f);			// ���������ߵ�P3���Ƶ�

    // NOTE: Change this to more efficient. Work in only the x,y plane, ignore z
    float3 grassBladeDir = normalize(p3);			// ��Ҷ����б����(XYƽ��)
    float3 bezierCtrlDir = normalize(cross(grassBladeDir, float3(0.0f, 0.0f, 1.0f)));	// ���������Ʒ���

    float3 p1 = 0.33f * p3;	// ���������ߵ�P1���Ƶ�(�ɿ�)
    float3 p2 = 0.66f * p3;	// ���������ߵ�P2���Ƶ�(�ɿ�)

    p1 += bezierCtrlDir * bend * PassDataCB.p1Flexibility;
    p2 += bezierCtrlDir * bend * PassDataCB.p2Flexibility;

    float p1Weight = 0.33f;
    float p2Weight = 0.66f;
    float p3Weight = 1.00f;

    float p1ffset = pow(p1Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((_Time + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p1Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength; 
    float p2ffset = pow(p2Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((_Time + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p2Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength;
    float p3ffset = pow(p3Weight, PassDataCB.wavePower) * (PassDataCB.waveAmplitude / 100.0f) * sin((_Time + hash * 2 * 3.1415) * PassDataCB.waveSpeed + p3Weight * 2 * 3.1415 * PassDataCB.sinOffsetRange) * windStrength; 
    p3ffset = (p3ffset) - PassDataCB.pushTipOscillationForward * mult * (pow(p3Weight, PassDataCB.wavePower) * PassDataCB.waveAmplitude / 100.0f) / 2.0f;

    p1 += bezierCtrlDir * p1ffset;
    p2 += bezierCtrlDir * p2ffset;
    p3 += bezierCtrlDir * p3ffset;

    // Evaluate Bezier curve(ignore Z, only XYƽ��)
    float3 midPoint         = cubicBezier(p0, p1, p2, p3, t);                       // �����������ϵĵ�(midPoint����Ϊ���ɸõ������ڵ���������ҶƬ��������)
    float3 midPointTangent  = normalize(bezierTangent(p0, p1, p2, p3, t));          // �õ������
    float3 midPointNormal   = normalize(cross(tangent, float3(0.0f, 0.0f, 1.0f)));  // �õ�ķ���

    // ���㵱ǰ�㴦ҶƬ�Ŀ��
    float  currbladeWidth = grassBlade.width * (1.0f - PassDataCB.widthTaperAmount * t);    // ����t������(Ҳ���Ǹ��ӽӽ�Ҷ��)��ҶƬ�Ŀ��Ӧ�ü�С
    // ����ҶƬ���ϵĵ�
    float3 sidePoint = float3(midPoint.x, midPoint.y, midPoint.z + currbladeWidth * side);
    // ����õ�ķ���
    float3 sidePointNormal = midPointNormal;
    sidePointNormal.z += currbladeWidth * side;
    sidePointNormal = normalize(sidePointNormal);


    float angle = blade.rotAngle;

    float3x3 facingRotateMat = AngleAxis3x3(-angle, float3(0.0f, 1.0f, 0.0f));
    float3x3 sideRotateMat   = AngleAxis3x3(sideCurve, normalize(midPointTangent));

    // ʩ��ҶƬ�ıߵ���ת
    float3 vectorFromMidToSide = sidePoint - midPoint;
    vectorFromMidToSide = mul(sideRotateMat, vectorFromMidToSide);
    sidePoint = midPoint + vectorFromMidToSide;

    midPointNormal  = mul(sideRotateMat, midPointNormal);
    sidePointNormal = mul(sideRotateMat, sidePointNormal);
    
    // ʩ��Facing��ת
    sidePoint       = mul(facingRotateMat, sidePoint);
    midPointNormal  = mul(facingRotateMat, midPointNormal);
    sidePointNormal = mul(facingRotateMat, sidePointNormal);

    // �ƶ�����Ӧ������������
    float3 finalPosition = sidePoint + bladePosition;


}

p2o PSMain(v2p input) {
	
}

#endif